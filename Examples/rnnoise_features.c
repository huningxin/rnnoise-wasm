#include <stdio.h>
#include <stdlib.h>
#include "rnnoise.h"
#include <emscripten.h>

#define FRAME_SIZE 480
#define FEATURE_SIZE 42
#define WEB_SAMEPLE_RATE 44100
#define RN_SAMPLE_RATE 48000

uint64_t Resample_f32(const float *input, float *output,
                      int inSampleRate, int outSampleRate,
                      uint64_t inputSize, uint32_t channels) {
    if (input == NULL)
        return 0;
    uint64_t outputSize = inputSize * outSampleRate / inSampleRate;
    if (output == NULL)
        return outputSize;
    double stepDist = ((double) inSampleRate / (double) outSampleRate);
    const uint64_t fixedFraction = (1LL << 32);
    const double normFixed = (1.0 / (1LL << 32));
    uint64_t step = ((uint64_t) (stepDist * fixedFraction + 0.5));
    uint64_t curOffset = 0;
    for (uint32_t i = 0; i < outputSize; i += 1) {
        for (uint32_t c = 0; c < channels; c += 1) {
            *output++ = (float) (input[c] + (input[c + channels] - input[c]) * (
                    (double) (curOffset >> 32) + ((curOffset & (fixedFraction - 1)) * normFixed)
            )
            );
        }
        curOffset += step;
        input += (curOffset >> 32) * channels;
        curOffset &= (fixedFraction - 1);
    }
    return outputSize;
}

float* EMSCRIPTEN_KEEPALIVE get_features(float *input) {
  float x[FRAME_SIZE];
  float y[FEATURE_SIZE];
  DenoiseState *st;
  st = rnnoise_create(NULL);
  int rnpcm_length = 0;
  int silence = 0;
  int input_size = WEB_SAMEPLE_RATE;
  float* rnpcm_1s_16bit = (float *) malloc(sizeof(*rnpcm_1s_16bit) * WEB_SAMEPLE_RATE *2);
  float* features_1s = (float *) malloc(sizeof(*features_1s) * FEATURE_SIZE * 100);

  for (int i = 0; i < input_size; i++) {
    input[i] = input[i] * 32768.0f;
  }

  rnpcm_length = Resample_f32(input, rnpcm_1s_16bit, WEB_SAMEPLE_RATE, RN_SAMPLE_RATE, input_size, 1);
  int frames_nums = rnpcm_length / FRAME_SIZE;

  for (int n = 0; n < frames_nums; n++) {
    for (int i = 0; i < FRAME_SIZE; i++) {
      x[i] = rnpcm_1s_16bit[i + n*FRAME_SIZE];
    }
    silence = get_frame_features(st, x, y);
    for (int i = 0; i < FEATURE_SIZE; i++) {
      features_1s[i + n*FEATURE_SIZE] = y[i];
    }
  }
  rnnoise_destroy(st);
  return features_1s;
}
