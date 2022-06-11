#ifndef AUDIO_H
#define AUDIO_H

#include <memory>
#include <vector>

namespace rtaudio {

struct AudioFrame {
  explicit AudioFrame(size_t buffer_size);

  std::vector<float> samples;
  std::vector<float> fft;
  std::vector<float> absolute_fft;
  float rms = 0;
  float rms_slow_max = 0;
  float rms_slow = 0;
  float rms_mid = 0;
  float rms_fast = 0;
  float normalized_rms = 0;
  float normalized_rms_mid = 0;
  float normalized_rms_fast = 0;
  float peak = 0;
  float peak_slow = 0;
  float peak_mid = 0;
  float peak_fast = 0;
  float normalized_peak = 0;
  float normalized_peak_mid = 0;
  float normalized_peak_fast = 0;

  struct Band {
    explicit Band(size_t buffer_size);

    std::vector<float> samples;
    float rms = 0;
    float rms_slow_max = 0;
    float rms_slow = 0;
    float rms_mid = 0;
    float rms_fast = 0;
    float normalized_rms = 0;
    float normalized_rms_mid = 0;
    float normalized_rms_fast = 0;
    float peak = 0;
    float peak_slow = 0;
    float peak_mid = 0;
    float peak_fast = 0;
    float normalized_peak = 0;
    float normalized_peak_mid = 0;
    float normalized_peak_fast = 0;
  };

  Band bass;
  Band mid;
  Band high;
};

class AudioProcessor {
 public:
  virtual ~AudioProcessor() = default;
  virtual void Process(AudioFrame* frame) = 0;
};

struct AudioProcessorOptions {
  size_t buffer_size;
  float sample_rate;
};

std::unique_ptr<AudioProcessor> CreateAudioProcessor(
    AudioProcessorOptions options);

}  // namespace rtaudio

#endif  // AUDIO_H
