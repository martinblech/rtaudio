#include "device_info.h"

#include <portaudio.h>

#include "pacheck.h"

namespace rtaudio {

Napi::Object GetDeviceInfo(Napi::Env env, int device_index) {
  const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(device_index);
  Napi::Object device = Napi::Object::New(env);
  device["id"] = device_index;
  device["name"] = deviceInfo->name;
  device["maxInputChannels"] = deviceInfo->maxInputChannels;
  device["maxOutputChannels"] = deviceInfo->maxOutputChannels;
  device["defaultSampleRate"] = deviceInfo->defaultSampleRate;
  device["defaultLowInputLatency"] = deviceInfo->defaultLowInputLatency;
  device["defaultLowOutputLatency"] = deviceInfo->defaultLowOutputLatency;
  device["defaultHighInputLatency"] = deviceInfo->defaultHighInputLatency;
  device["defaultHighOutputLatency"] = deviceInfo->defaultHighOutputLatency;
  device["hostAPIName"] = Pa_GetHostApiInfo(deviceInfo->hostApi)->name;
  return device;
}

Napi::Value GetDevices(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  PA_CHECK(Pa_Initialize(), env.Null());
  const PaDeviceIndex device_count = Pa_GetDeviceCount();
  Napi::Array devices = Napi::Array::New(env);
  for (PaDeviceIndex i = 0; i < device_count; ++i) {
    devices[i] = GetDeviceInfo(env, i);
  }
  PA_CHECK(Pa_Terminate(), env.Null());
  return devices;
}

}  // namespace rtaudio
