#include "fft.h"

#include <math.h>

#include <cstring>
#include <iostream>

namespace rtaudio {

RealFFT::RealFFT(int size)
    : size_(size),
      pffft_setup_(pffft_new_setup(size, PFFFT_REAL)),
      input_((float*)pffft_aligned_malloc(size * sizeof(float))),
      work_((float*)pffft_aligned_malloc(size * sizeof(float))),
      output_((float*)pffft_aligned_malloc(size * sizeof(float))) {}

RealFFT::~RealFFT() {
  pffft_aligned_free(output_);
  pffft_aligned_free(work_);
  pffft_aligned_free(input_);
  pffft_destroy_setup(pffft_setup_);
}

void RealFFT::ForwardTransform(const float* input, float* output) {
  std::memcpy(input_, input, size_ * sizeof(float));
  pffft_transform_ordered(pffft_setup_, input_, output_, work_, PFFFT_FORWARD);
  std::memcpy(output, output_, size_ * sizeof(float));
  output[1] = output[size_ + 1] = 0;
  output[size_] = abs(output_[1]);
}

void RealFFT::ForwardTransform(const std::vector<float>& input,
                               std::vector<float>* output) {
  if (input.size() != size_) {
    std::cerr << "input->size() != size_" << std::endl;
    return;
  }
  if (output->size() != size_ + 2) {
    std::cerr << "output->size() != size_ + 2" << std::endl;
    return;
  }
  ForwardTransform(input.data(), output->data());
}

void RealFFT::GetAbsolute(const std::vector<float>& input,
                          std::vector<float>* output) {
  if (output->size() != input.size() / 2) {
    std::cerr << "output->size() != input.size() / 2" << std::endl;
    return;
  }
  RealFFT::GetAbsolute(input.data(), input.size(), output->data());
}

void RealFFT::GetAbsolute(const float* input, size_t input_size,
                          float* output) {
  for (size_t i = 0; i < input_size / 2 + 1; ++i) {
    float real = input[2 * i];
    float img = input[2 * i + 1];
    output[i] = sqrt(real * real + img * img);
  }
}

}  // namespace rtaudio
