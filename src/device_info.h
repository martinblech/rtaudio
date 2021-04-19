#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <napi.h>

namespace rtaudio {

Napi::Value GetDevices(const Napi::CallbackInfo& info);

}  // namespace rtaudio

#endif  // DEVICE_INFO_H
