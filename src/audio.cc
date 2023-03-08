#include "audio.h"

#include <cmath>
#include <functional>
#include <optional>

#include "Iir.h"
#include "fft.h"

namespace rtaudio {
namespace {

float TauToAlpha(float sample_rate, float tau_seconds) {
  return 1.0 - std::exp(-1.0 / (sample_rate * tau_seconds));
}

class ExpDecayFollower {
 public:
  enum class Mode { kMax, kAvg };
  explicit ExpDecayFollower(float sample_rate, float tau_seconds, Mode mode)
      : alpha_(TauToAlpha(sample_rate, tau_seconds)), mode_(mode) {}

  float update(float value) {
    if (!value_) {
      value_ = value;
      return value;
    }
    if (value > *value_ && mode_ == Mode::kMax) {
      value_ = value;
    } else {
      *value_ += alpha_ * (value - *value_);
    }
    return *value_;
  }

  float value() const { return *value_; }

 private:
  std::optional<float> value_;
  const float alpha_;
  const Mode mode_;
};

float GetRms(const std::vector<float>& samples) {
  if (samples.empty()) {
    return 0;
  }
  float sum = 0;
  for (const float sample : samples) {
    sum += sample * sample;
  }
  return std::sqrt(sum / samples.size());
}

float GetPeak(const std::vector<float>& samples) {
  float peak = 0;
  for (const float sample : samples) {
    const float abs_sample = std::abs(sample);
    if (abs_sample > peak) {
      peak = abs_sample;
    }
  }
  return peak;
}

class CompositeProcessor final : public AudioProcessor {
 public:
  explicit CompositeProcessor(
      std::vector<std::unique_ptr<AudioProcessor>> processors)
      : processors_(std::move(processors)) {}

  void Process(AudioFrame* frame) final {
    for (auto& processor : processors_) {
      processor->Process(frame);
    }
  };

 private:
  std::vector<std::unique_ptr<AudioProcessor>> processors_;
};

class FFTProcessor final : public AudioProcessor {
 public:
  explicit FFTProcessor(size_t buffer_size) : real_fft_(buffer_size) {}
  void Process(AudioFrame* frame) final {
    // FFT
    real_fft_.ForwardTransform(frame->samples, &frame->fft);
    RealFFT::GetAbsolute(frame->fft, &frame->absolute_fft);
  };

 private:
  RealFFT real_fft_;
};

class BandProcessor final : public AudioProcessor {
 public:
  explicit BandProcessor(size_t buffer_size, float sample_rate) {
    static const float kBassCutoff = 250;
    static const float kMidCenter = 1350;
    static const float kMidWidth = 1500;
    static const float kHighCutoff = 6000;
    bass_filter_.setup(sample_rate, kBassCutoff);
    mid_filter_.setup(sample_rate, kMidCenter, kMidWidth);
    high_filter_.setup(sample_rate, kHighCutoff);
  }
  void Process(AudioFrame* frame) final {
    for (size_t i = 0; i < frame->samples.size(); ++i) {
      const float sample = frame->samples[i];
      frame->bass.samples[i] = bass_filter_.filter(sample);
      frame->mid.samples[i] = mid_filter_.filter(sample);
      frame->high.samples[i] = high_filter_.filter(sample);
    }
  };

 private:
  Iir::Butterworth::LowPass<4> bass_filter_;
  Iir::Butterworth::BandPass<4> mid_filter_;
  Iir::Butterworth::HighPass<4> high_filter_;
};

class PeakProcessor final : public AudioProcessor {
 public:
  explicit PeakProcessor(size_t buffer_size, float sample_rate) {
    const float sr = sample_rate / buffer_size;  // Follower sample rate.
    static const float kTauSlow = 2 * 60;        // 2 minutes.
    static const float kTauMid = .75;            // 750 milliseconds.
    static const float kTauFast = .1;            // 100 milliseconds.
    using Mode = ExpDecayFollower::Mode;
    followers_.emplace_back(
        [](const AudioFrame* frame) { return frame->rms; },
        [](AudioFrame* frame, float value) { frame->rms_slow_max = value; },
        ExpDecayFollower(sr, kTauSlow, Mode::kMax));
    for (const auto& [mode, in, out_slow, out_mid, out_fast] : {
             std::make_tuple(Mode::kMax, &AudioFrame::peak,
                             &AudioFrame::peak_slow, &AudioFrame::peak_mid,
                             &AudioFrame::peak_fast),
             std::make_tuple(Mode::kAvg, &AudioFrame::rms,
                             &AudioFrame::rms_slow, &AudioFrame::rms_mid,
                             &AudioFrame::rms_fast),
         }) {
      followers_.emplace_back(
          [in = in](const AudioFrame* frame) { return frame->*in; },
          [out = out_slow](AudioFrame* frame, float value) {
            frame->*out = value;
          },
          ExpDecayFollower(sr, kTauSlow, mode));
      followers_.emplace_back(
          [in = in](const AudioFrame* frame) { return frame->*in; },
          [out = out_mid](AudioFrame* frame, float value) {
            frame->*out = value;
          },
          ExpDecayFollower(sr, kTauMid, mode));
      followers_.emplace_back(
          [in = in](const AudioFrame* frame) { return frame->*in; },
          [out = out_fast](AudioFrame* frame, float value) {
            frame->*out = value;
          },
          ExpDecayFollower(sr, kTauFast, mode));
    }
    for (const auto& band :
         {&AudioFrame::bass, &AudioFrame::mid, &AudioFrame::high}) {
      followers_.emplace_back(
          [band = band](const AudioFrame* frame) { return (frame->*band).rms; },
          [band = band](AudioFrame* frame, float value) {
            (frame->*band).rms_slow_max = value;
          },
          ExpDecayFollower(sr, kTauSlow, Mode::kMax));
      for (const auto& [mode, in, out_slow, out_mid, out_fast] : {
               std::make_tuple(Mode::kMax, &AudioFrame::Band::peak,
                               &AudioFrame::Band::peak_slow,
                               &AudioFrame::Band::peak_mid,
                               &AudioFrame::Band::peak_fast),
               std::make_tuple(Mode::kAvg, &AudioFrame::Band::rms,
                               &AudioFrame::Band::rms_slow,
                               &AudioFrame::Band::rms_mid,
                               &AudioFrame::Band::rms_fast),
           }) {
        followers_.emplace_back(
            [band = band, in = in](const AudioFrame* frame) {
              return frame->*band.*in;
            },
            [band = band, out = out_slow](AudioFrame* frame, float value) {
              frame->*band.*out = value;
            },
            ExpDecayFollower(sr, kTauSlow, mode));
        followers_.emplace_back(
            [band = band, in = in](const AudioFrame* frame) {
              return frame->*band.*in;
            },
            [band = band, out = out_mid](AudioFrame* frame, float value) {
              frame->*band.*out = value;
            },
            ExpDecayFollower(sr, kTauMid, mode));
        followers_.emplace_back(
            [band = band, in = in](const AudioFrame* frame) {
              return frame->*band.*in;
            },
            [band = band, out = out_fast](AudioFrame* frame, float value) {
              frame->*band.*out = value;
            },
            ExpDecayFollower(sr, kTauFast, mode));
      }
    }
  }

  void Process(AudioFrame* frame) final {
    frame->rms = GetRms(frame->samples);
    frame->peak = GetPeak(frame->samples);
    for (AudioFrame::Band* band : {&frame->bass, &frame->mid, &frame->high}) {
      band->rms = GetRms(band->samples);
      band->peak = GetPeak(band->samples);
    }
    for (auto& [getter, setter, follower] : followers_) {
      float value = follower.update(getter(frame));
      setter(frame, value);
    }
  }

 private:
  using Getter = std::function<float(const AudioFrame*)>;
  using Setter = std::function<void(AudioFrame*, float value)>;

  std::vector<std::tuple<Getter, Setter, ExpDecayFollower>> followers_;
};

class NormalizeProcessor final : public AudioProcessor {
 public:
  explicit NormalizeProcessor() {}

  void Process(AudioFrame* frame) final {
    frame->normalized_rms = div(frame->rms, frame->rms_slow_max);
    frame->normalized_rms_mid = div(frame->rms_mid, frame->rms_slow_max);
    frame->normalized_rms_fast = div(frame->rms_fast, frame->rms_slow_max);
    frame->normalized_peak =
        div(frame->peak - frame->rms_slow, frame->peak_slow - frame->rms_slow);
    frame->normalized_peak_mid = div(frame->peak_mid - frame->rms_slow,
                                     frame->peak_slow - frame->rms_slow);
    frame->normalized_peak_fast = div(frame->peak_fast - frame->rms_slow,
                                      frame->peak_slow - frame->rms_slow);
    for (const auto& band_property :
         {&AudioFrame::bass, &AudioFrame::mid, &AudioFrame::high}) {
      AudioFrame::Band* band = &(frame->*band_property);
      band->normalized_rms = div(band->rms, band->rms_slow_max);
      band->normalized_rms_mid = div(band->rms_mid, band->rms_slow_max);
      band->normalized_rms_fast = div(band->rms_fast, band->rms_slow_max);
      band->normalized_peak =
          div(band->peak - band->rms_slow, band->peak_slow - band->rms_slow);
      band->normalized_peak_mid = div(band->peak_mid - band->rms_slow,
                                      band->peak_slow - band->rms_slow);
      band->normalized_peak_fast = div(band->peak_fast - band->rms_slow,
                                       band->peak_slow - band->rms_slow);
    }
  };

 private:
  static float div(float num, float div) {
    if (num <= 0 || div <= 0) {
      return 0;
    }
    return num / div;
  }
};

}  // namespace

AudioFrame::AudioFrame(size_t buffer_size)
    : samples(buffer_size, 0),
      fft(buffer_size + 2, 0),
      absolute_fft(fft.size() / 2, 0),
      bass(buffer_size),
      mid(buffer_size),
      high(buffer_size) {}

AudioFrame::Band::Band(size_t buffer_size) : samples(buffer_size, 0) {}

std::unique_ptr<AudioProcessor> CreateAudioProcessor(
    AudioProcessorOptions options) {
  std::vector<std::unique_ptr<AudioProcessor>> processors;
  processors.emplace_back(new FFTProcessor(options.buffer_size));
  processors.emplace_back(
      new BandProcessor(options.buffer_size, options.sample_rate));
  processors.emplace_back(
      new PeakProcessor(options.buffer_size, options.sample_rate));
  processors.emplace_back(new NormalizeProcessor());
  return std::unique_ptr<AudioProcessor>(
      new CompositeProcessor(std::move(processors)));
}

}  // namespace rtaudio
