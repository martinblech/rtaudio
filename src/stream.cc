#include "stream.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <utility>

#include "pacheck.h"

namespace rtaudio {
namespace {}  // namespace

Napi::Function InputStream::GetClass(Napi::Env env) {
  return DefineClass(
      env, "InputStream",
      {
          InputStream::InstanceMethod("start", &InputStream::Start),
          InputStream::InstanceMethod("stop", &InputStream::Stop),
      });
}

InputStream::InputStream(const Napi::CallbackInfo& info) : ObjectWrap(info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    NAPI_THROW(Napi::TypeError::New(env, "Missing options argument"));
  }

  if (!info[0].IsObject()) {
    NAPI_THROW(Napi::TypeError::New(env, "options argument is not an object"));
  }

  const Napi::Object options = info[0].As<Napi::Object>();
  if (const Napi::Value value = options["device"]; !value.IsUndefined()) {
    if (!value.IsNumber()) {
      NAPI_THROW(
          Napi::Error::New(env, std::string("Invalid value for device: ") +
                                    value.ToString().Utf8Value()));
    }
    device_ = value.ToNumber().Int32Value();
  }
  if (const Napi::Value value = options["sampleRate"]; !value.IsUndefined()) {
    if (!value.IsNumber()) {
      NAPI_THROW(
          Napi::Error::New(env, std::string("Invalid value for sampleRate: ") +
                                    value.ToString().Utf8Value()));
    }
    sample_rate_ = value.ToNumber().DoubleValue();
  } else {
    sample_rate_ = 44100;
  }
  if (const Napi::Value value = options["bufferSize"]; !value.IsUndefined()) {
    if (!value.IsNumber()) {
      NAPI_THROW(
          Napi::Error::New(env, std::string("Invalid value for bufferSize: ") +
                                    value.ToString().Utf8Value()));
    }
    buffer_size_ = value.ToNumber().Uint32Value();
  } else {
    buffer_size_ = 512;
  }
  if (const Napi::Value value = options["callback"]; !value.IsUndefined()) {
    if (!value.IsFunction()) {
      NAPI_THROW(
          Napi::Error::New(env, std::string("Invalid value for callback: ") +
                                    value.ToString().Utf8Value()));
    }
    callback_ = Napi::Persistent(value.As<Napi::Function>());
  }
  frame_ = std::unique_ptr<AudioFrame>(new AudioFrame(buffer_size_));
  processor_ = CreateAudioProcessor({
      .sample_rate = static_cast<float>(sample_rate_),
      .buffer_size = buffer_size_,
  });
  Napi::Object frame = Napi::Object::New(env);
  frame["sampleRate"] = Napi::Number::New(env, sample_rate_);
  frame["samples"] = Napi::Float32Array::New(env, frame_->samples.size());
  frame["fft"] = Napi::Float32Array::New(env, frame_->fft.size());
  frame["absoluteFft"] =
      Napi::Float32Array::New(env, frame_->absolute_fft.size());
  for (const auto band_name : {"bass", "mid", "high"}) {
    Napi::Object band = Napi::Object::New(env);
    band["samples"] = Napi::Float32Array::New(env, frame_->samples.size());
    frame[band_name] = band;
  }
  js_frame_ = Napi::Persistent(frame);
}

InputStream::~InputStream() {
  if (running_) {
    std::cerr << "~InputStream() destructor called while still running."
              << std::endl;
    tsfn_.Abort();
    Terminate();
  }
  running_ = false;
  if (reader_thread_.joinable()) {
    reader_thread_.join();
  }
}

void InputStream::Terminate() {
  PaError error;
  error = Pa_Terminate();
  if (error != paNoError) {
    std::cerr << "Pa_Terminate() failed: " << Pa_GetErrorText(error)
              << std::endl;
  }
}

void InputStream::Start(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (stream_) {
    NAPI_THROW(Napi::Error::New(env, "Stream already initialized"));
  }

  PA_CHECK(Pa_Initialize());

  struct Cleanup {
    bool success = false;
    ~Cleanup() {
      if (!success) {
        Terminate();
      }
    }
  } cleanup;

  if (!device_) {
    device_ = Pa_GetDefaultInputDevice();
  }

  const PaDeviceInfo* const inputInfo = Pa_GetDeviceInfo(*device_);
  if (!inputInfo) {
    NAPI_THROW(Napi::Error::New(
        env, std::string("Invalid device index: ") + std::to_string(*device_)));
  }
  if (inputInfo->maxInputChannels == 0) {
    NAPI_THROW(Napi::Error::New(
        env, std::string("Not an input device: ") + inputInfo->name));
  }
  const PaStreamParameters inputParameters{
      .device = *device_,
      .channelCount = 1,
      .sampleFormat = paFloat32,
      .suggestedLatency = inputInfo->defaultLowInputLatency,
  };
  PA_CHECK(Pa_OpenStream(&stream_, &inputParameters, nullptr, sample_rate_,
                         buffer_size_, paNoFlag, nullptr, nullptr));
  PA_CHECK(Pa_StartStream(stream_));

  static const char kResourceName[] = "Audio Frame Callback";
  static constexpr size_t kMaxQueueSize = 1;
  static constexpr size_t kInitialThreadCount = 1;
  if (!callback_.IsEmpty()) {
    tsfn_ = TSFN::New(env, callback_.Value().As<Napi::Function>(),
                      kResourceName, kMaxQueueSize, kInitialThreadCount, this);
  } else {
    tsfn_ =
        TSFN::New(env, kResourceName, kMaxQueueSize, kInitialThreadCount, this);
  }

  running_.store(true);
  reader_thread_ = std::thread([this, js_this = Napi::Persistent(info.This())] {
    auto last_available = std::chrono::system_clock::now();
    while (running_.load()) {
      auto current_time = std::chrono::system_clock::now();
      signed long available = Pa_GetStreamReadAvailable(stream_);
      if (available < 0) {
        PaError status = available;
        std::ostringstream message;
        message << "Error reading stream: " << Pa_GetErrorText(status);
        error_ = message.str();
        tsfn_.BlockingCall();
        break;
      }
      if (available == 0) {
        auto elapsed = current_time - last_available;
        if (elapsed > std::chrono::seconds(1)) {
          error_ = "Timeout: over 1s waiting for audio";
          tsfn_.BlockingCall();
          break;
        }
        continue;
      }
      last_available = current_time;
      PaError status =
          Pa_ReadStream(stream_, frame_->samples.data(), buffer_size_);
      overflowed_ = false;
      if (status == paInputOverflowed) {
        overflowed_ = true;
        std::cerr << "Input overflowed: " << Pa_GetErrorText(status)
                  << std::endl;
      } else if (status != paNoError) {
        std::ostringstream message;
        message << "Error reading stream: " << Pa_GetErrorText(status);
        error_ = message.str();
        tsfn_.BlockingCall();
        break;
      }
      processor_->Process(frame_.get());
      tsfn_.BlockingCall();
    }
    tsfn_.Release();
    running_ = false;
    if (error_) {
      Terminate();
    }
  });
  cleanup.success = true;
}

void InputStream::UpdateJsFrame(Napi::Env env) {
  Napi::Object frame = js_frame_.Value();
  frame["overflowed"] = Napi::Boolean::New(env, overflowed_);
  Napi::Float32Array samples = frame.Get("samples").As<Napi::Float32Array>();
  memcpy(samples.Data(), frame_->samples.data(),
         sizeof(float) * frame_->samples.size());
  Napi::Float32Array fft = frame.Get("fft").As<Napi::Float32Array>();
  memcpy(fft.Data(), frame_->fft.data(), sizeof(float) * frame_->fft.size());
  Napi::Float32Array absolute_fft =
      frame.Get("absoluteFft").As<Napi::Float32Array>();
  memcpy(absolute_fft.Data(), frame_->absolute_fft.data(),
         sizeof(float) * frame_->absolute_fft.size());
  frame["rms"] = Napi::Number::New(env, frame_->rms);
  frame["rmsSlow"] = Napi::Number::New(env, frame_->rms_slow);
  frame["rmsMid"] = Napi::Number::New(env, frame_->rms_mid);
  frame["rmsFast"] = Napi::Number::New(env, frame_->rms_fast);
  frame["normalizedRmsMid"] =
      Napi::Number::New(env, frame_->normalized_rms_mid);
  frame["normalizedRmsFast"] =
      Napi::Number::New(env, frame_->normalized_rms_fast);
  frame["peak"] = Napi::Number::New(env, frame_->peak);
  frame["peakSlow"] = Napi::Number::New(env, frame_->peak_slow);
  frame["peakMid"] = Napi::Number::New(env, frame_->peak_mid);
  frame["peakFast"] = Napi::Number::New(env, frame_->peak_fast);
  frame["normalizedPeakMid"] =
      Napi::Number::New(env, frame_->normalized_peak_mid);
  frame["normalizedPeakFast"] =
      Napi::Number::New(env, frame_->normalized_peak_fast);
  for (auto& [band_name, band] : {
           std::make_pair("bass", &frame_->bass),
           std::make_pair("mid", &frame_->mid),
           std::make_pair("high", &frame_->high),
       }) {
    Napi::Object js_band = frame.Get(band_name).As<Napi::Object>();
    Napi::Float32Array samples =
        js_band.Get("samples").As<Napi::Float32Array>();
    memcpy(samples.Data(), band->samples.data(),
           sizeof(float) * band->samples.size());
    js_band["rms"] = Napi::Number::New(env, band->rms);
    js_band["rmsSlow"] = Napi::Number::New(env, band->rms_slow);
    js_band["rmsMid"] = Napi::Number::New(env, band->rms_mid);
    js_band["rmsFast"] = Napi::Number::New(env, band->rms_fast);
    js_band["normalizedRmsMid"] =
        Napi::Number::New(env, band->normalized_rms_mid);
    js_band["normalizedRmsFast"] =
        Napi::Number::New(env, band->normalized_rms_fast);
    js_band["peak"] = Napi::Number::New(env, band->peak);
    js_band["peakSlow"] = Napi::Number::New(env, band->peak_slow);
    js_band["peakMid"] = Napi::Number::New(env, band->peak_mid);
    js_band["peakFast"] = Napi::Number::New(env, band->peak_fast);
    js_band["normalizedPeakMid"] =
        Napi::Number::New(env, band->normalized_peak_mid);
    js_band["normalizedPeakFast"] =
        Napi::Number::New(env, band->normalized_peak_fast);
  }
}

void InputStream::CallJs(Napi::Env env, Napi::Function callback,
                         InputStream* stream, void* data) {
  if (env == nullptr || callback == nullptr) {
    return;
  }
  stream->UpdateJsFrame(env);
  if (!stream->error_) {
    Napi::Object frame = stream->js_frame_.Value();
    callback.Call({env.Undefined(), frame});
  } else {
    callback.Call(
        {Napi::Error::New(env, *stream->error_).Value(), env.Undefined()});
    stream->error_ = std::nullopt;
  }
}

void InputStream::Stop(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!stream_) {
    NAPI_THROW(Napi::Error::New(env, "Stream not initialized"));
  }

  if (!running_.load()) {
    NAPI_THROW(Napi::Error::New(env, "Reader thread never started"));
  }

  if (!reader_thread_.joinable()) {
    NAPI_THROW(Napi::Error::New(env, "Reader thread not running"));
  }
  tsfn_.Abort();

  running_.store(false);
  reader_thread_.join();
  reader_thread_ = std::thread();

  PA_CHECK(Pa_StopStream(stream_));
  stream_ = nullptr;
  Terminate();
}

}  // namespace rtaudio
