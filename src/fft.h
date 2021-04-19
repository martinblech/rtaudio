#ifndef FFT_H
#define FFT_H

#include <vector>

#include "pffft.h"

namespace rtaudio {

class RealFFT {
 public:
  RealFFT(int size);
  ~RealFFT();
  void ForwardTransform(const std::vector<float>& input,
                        std::vector<float>* output);
  static void GetAbsolute(const std::vector<float>& input,
                          std::vector<float>* output);

 private:
  void ForwardTransform(const float* input, float* output);
  static void GetAbsolute(const float* input, size_t input_size, float* output);

  int size_;
  PFFFT_Setup* pffft_setup_;
  float* input_;
  float* work_;
  float* output_;
};

}  // namespace rtaudio

#endif  // FFT_H
