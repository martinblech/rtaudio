#ifndef PACHECK_H
#define PACHECK_H

#define PA_CHECK(call, ...)                                                    \
  do {                                                                         \
    PaError error = (call);                                                    \
    if (error != paNoError) {                                                  \
      NAPI_THROW(Napi::Error::New(                                             \
                     env, std::string(#call) + ": " + Pa_GetErrorText(error)), \
                 __VA_ARGS__);                                                 \
    }                                                                          \
  } while (0)

#endif  // PACHECK_H
