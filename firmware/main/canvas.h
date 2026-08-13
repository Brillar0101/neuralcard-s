// Turns an air-written gesture into an MNIST-shaped 28x28 image.
//
// This is the piece that lets a model trained on pen-on-paper digits read a
// digit drawn in the air, and it is the weakest link in the chain. The network
// is fixed and known good; how faithfully a waved card reproduces an MNIST
// glyph is the open question, and it is what to tune on real hardware.
//
// Path: gyro -> angular displacement -> 2D points -> normalise like MNIST ->
// rasterise a thick polyline.
//
// Integrating the gyro is deliberate. Tracking translation would mean double
// integrating the accelerometer, and that drifts quadratically; a one second
// gesture would smear. Angular displacement integrates once, so drift grows
// linearly and stays usable over the two seconds a digit takes to draw.
//
// No ESP-IDF headers here, so this can be exercised on a host.
#pragma once
#include <stdint.h>

#define CANVAS_MAX_POINTS 512

typedef struct {
    float x[CANVAS_MAX_POINTS];
    float y[CANVAS_MAX_POINTS];
    int   n;
} canvas_path_t;

// Reset a path and the integrator state.
void canvas_begin(canvas_path_t *p);

// Feed one gyro sample, degrees per second, dt in seconds. Bias should already
// be removed. Appends a point; silently ignores samples past the buffer.
void canvas_add_sample(canvas_path_t *p, float gx, float gy, float gz, float dt);

// Rasterise into a 28x28 float image, 0..1, ink high, using MNIST framing:
// scale the longest side into a 20x20 box preserving aspect, then place it so
// the ink's centre of mass sits at the centre of the 28x28 field.
// Returns 0 if the path is too short or too small to be a digit.
int canvas_render(const canvas_path_t *p, float img[28 * 28]);
