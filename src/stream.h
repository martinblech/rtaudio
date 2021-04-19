#ifndef STREAM_H
#define STREAM_H

#include <napi.h>
#include <portaudio.h>

#include <atomic>
#include <memory>
#include <thread>

#include "audio.h"

namespace rtaudio {

class InputStream : public Napi::ObjectWrap<InputStream> {
 public:
  static Napi::Function GetClass(Napi::Env);
  InputStream(const Napi::CallbackInfo&);
  ~InputStream();

  void Start(const Napi::CallbackInfo&);

  void Stop(const Napi::CallbackInfo&);

 private:
  static void CallJs(Napi::Env env, Napi::Function callback,
                     InputStream* stream, void* data);
  void UpdateJsFrame(Napi::Env env);

  PaStream* stream_ = nullptr;
  std::atomic<bool> running_{false};
  int device_;
  double sample_rate_;
  unsigned long buffer_size_;
  bool overflowed_;
  std::unique_ptr<AudioFrame> frame_;
  std::unique_ptr<AudioProcessor> processor_;
  std::thread reader_thread_;

  using TSFN = Napi::TypedThreadSafeFunction<InputStream, void, CallJs>;

  TSFN tsfn_;
  Napi::FunctionReference callback_;
  Napi::Reference<Napi::Object> js_frame_;
};

}  // namespace rtaudio

#endif  // STREAM_H
