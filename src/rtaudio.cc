#include <napi.h>

#include "device_info.h"
#include "stream.h"

namespace rtaudio {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "getDevices"),
              Napi::Function::New(env, GetDevices));
  exports.Set(Napi::String::New(env, "InputStream"),
              InputStream::GetClass(env));
  return exports;
}

NODE_API_MODULE(rtaudio, Init)

}  // namespace rtaudio
