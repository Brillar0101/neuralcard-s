// Inference for the pretrained MNIST CNN. No training, no framework.
//
// The weights in mnist_weights.h come from the ONNX Model Zoo. This file is
// the forward pass for them, written out longhand so there is no runtime to
// port and nothing to link against.
//
// Deliberately free of ESP-IDF headers so the same code can be compiled and
// checked against real MNIST test digits on a host machine.
#pragma once
#include <stdint.h>

#define NN_HIDDEN_TAPS 8   // hidden activations surfaced for the LED row

typedef struct {
    float logits[10];             // raw scores, argmax of this is the answer
    float prob[10];               // softmax of logits, sums to 1
    float hidden[NN_HIDDEN_TAPS]; // pooled conv2 features, 0..1, for the display
    int   digit;                  // argmax
    float confidence;             // prob[digit]
} nn_result_t;

// Classify a 28x28 image. Pixels are 0..1, row major, ink is high, like MNIST.
void nn_infer(const float img[28 * 28], nn_result_t *out);
