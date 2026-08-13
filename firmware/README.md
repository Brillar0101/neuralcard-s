# NeuralCard firmware

Firmware for the v2.3 board. Nothing here is trained. The network is a
pretrained MNIST CNN taken off the shelf, and the work is in getting a digit
written in the air to look enough like an MNIST digit for it to read.

What works today:

- Charlieplex driver for all 24 LEDs, 8 brightness levels, mapping extracted
  from the board netlist (D1-D24 to CP pin pairs)
- LSM6DS3 driver over I2C (SDA=IO8, SCL=IO17, addr 0x6B)
- Air-writing capture, stroke rendering, and digit classification
- Gesture recorder, motion-triggered capture printed as CSV over the console
- Boot self-test: LED sweep and IMU WHO_AM_I check

Builds for esp32s3 at 255 KB, leaving 76% of the app partition free, and uses
about a third of DIRAM.

## Why there is no training step

The board's classifier is `mnist-8` from the ONNX Model Zoo, published by the
ONNX project under the MIT licence. It is a 5,998 parameter CNN:

    input   1 x 28 x 28, 0..1
    conv1   8 filters 5x5, pad 2  -> relu -> maxpool 2x2 stride 2
    conv2   16 filters 5x5, pad 2 -> relu -> maxpool 3x3 stride 3
    fc      256 -> 10 logits

That is 23.4 KB of float32 weights, small enough to compile straight into the
binary as a C array. `tools/export_mnist_weights.py` downloads the ONNX file
and writes `main/mnist_weights.h`. The forward pass in `main/nn.c` is written
longhand, so there is no inference runtime to port and nothing to link.

Measured, not assumed: `nn.c` scores 98.9% on all 10,000 MNIST test digits when
compiled on a host and run against the real test set, matching the reference
implementation prediction for prediction.

## How air-writing becomes an MNIST digit

`main/canvas.c` integrates the gyro into angular displacement and treats that
as pen position: yaw sweeps the pen left and right, pitch sweeps it up and
down. Roll is ignored, since twisting the card carries no position.

Integrating the gyro is a deliberate choice. Tracking translation instead would
mean double integrating the accelerometer, and that drifts quadratically, so a
one second gesture smears into nothing. Angular displacement integrates once,
so drift grows linearly and stays usable across the couple of seconds a digit
takes to draw.

The stroke is then framed the way MNIST itself is framed, because the network
has only ever seen digits positioned that way: scale the longer side of the
bounding box into a 20x20 box preserving aspect ratio, rasterise a soft-edged
polyline, then shift the result so the ink's centre of mass sits at the centre
of the 28x28 field.

Gyro bias changes with temperature, so it is measured fresh from 50 still
samples immediately before every stroke rather than calibrated once.

## What to actually expect

Two things have been measured and one has not.

Measured: the classifier is right 98.9% of the time on real MNIST. Measured:
the pipeline survives sensor noise. Feeding synthetic strokes through
`canvas.c` and `nn.c` with up to 20 deg/s of added gyro noise and 2 deg/s of
bias error still reads 95% to 100% of them correctly, so sensor quality is not
the limiting factor.

Not measured, and this is the real question: whether a digit drawn by a human
hand holding a card produces a shape the model recognises. Synthetic strokes
are idealised. Real ones will not be. Expect this to be the part that needs
tuning on hardware, and expect it to be worse than the numbers above.

Two known structural problems, both inherent to writing in the air rather than
faults in the model:

There is no pen lift in the air. A 4, 5, or 7 written with a separate stroke
becomes one connected scribble, which is not what MNIST contains. Writing those
digits in a single stroke works better.

The drawing plane is whatever the wrist decides. A digit written on a tilted
plane arrives sheared. Holding the card square to the body helps.

If the shape gap turns out too wide, the honest fixes stay training-free: tune
the stroke thickness and framing in `canvas.c`, or reject low-confidence reads
rather than guessing. The gesture recorder stays in the firmware so the gap can
be measured on real hardware instead of argued about.

## Build and flash

Requires ESP-IDF v5.x (`idf.py` on PATH, target esp32s3).

    cd firmware
    idf.py set-target esp32s3
    idf.py build
    idf.py flash monitor        # plain USB-C cable, no adapter

First flash of a blank board: hold BOOT, tap RST, release BOOT (printed on
the silk), then run the flash command.

To regenerate the weights header from the published model:

    pip install onnx numpy
    python3 tools/export_mnist_weights.py

## Serial commands

| Key | Action |
|---|---|
| `w` | air-write a digit and classify it |
| `p` | print the last stroke the network was shown, as ASCII art |
| `t` | LED sweep test |
| `s` | stream raw IMU CSV (any key stops) |
| `0`-`9` | record one gesture labelled with that digit |

`p` is the debugging tool that matters. When a digit reads wrong, it shows
whether the network saw a malformed shape or simply misjudged a good one.

## What the LEDs show

| LEDs | Row | Shows |
|---|---|---|
| D1-D6 | input | the six live IMU axes while you write |
| D7-D15 | hidden | pooled conv2 features from the classified stroke |
| D16-D24 | output | the ten class probabilities, brightest wins |

The hidden row is a readout, not part of the classification. The real second
convolution has 16 channels, and they are averaged down in pairs to fit the
eight LEDs the board has.

## One thing to verify on first light-up

`cp_set_outputs()` assumes D16-D24 are the output-column LEDs beside the 0-9
silk labels in order. The netlist cannot confirm visual order, so check it
against the physical board and permute the mapping if needed.
