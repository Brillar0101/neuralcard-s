// Capture an air-written digit and light the network up as it is read.
#pragma once
#include <stdbool.h>

// Waits for motion, records until the hand goes still, renders the stroke and
// classifies it with the pretrained CNN. Drives the LEDs throughout: the input
// row tracks the live gyro, the hidden row shows conv features, the output row
// holds the ten class probabilities.
//
// Returns the digit, or -1 if the gesture was too small to read.
int infer_capture_and_classify(void);

// Print the last captured stroke to the console as ASCII art, for checking
// what the network was actually shown.
void infer_dump_last_image(void);
