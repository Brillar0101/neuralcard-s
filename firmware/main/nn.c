// Forward pass for the pretrained MNIST CNN. See nn.h.
//
// Sizes are fixed at compile time, so every buffer is a plain static array and
// nothing is allocated at runtime. Peak working set is the two conv outputs,
// 8*28*28 + 16*14*14 floats, about 37 KB, which the S3 has room for in SRAM.
#include "nn.h"
#include "mnist_weights.h"
#include <math.h>
#include <string.h>

#define C1 8    // conv1 filters
#define C2 16   // conv2 filters
#define K  5    // both convs are 5x5, pad 2

static float c1_out[C1][28][28];
static float p1_out[C1][14][14];
static float c2_out[C2][14][14];
static float p2_out[C2][4][4];

// 5x5 convolution, pad 2, stride 1, followed by bias and relu.
static void conv_relu(const float *in, int in_ch, int side,
                      const float *w, const float *b, float *out, int out_ch)
{
    for (int o = 0; o < out_ch; o++) {
        float *dst = out + (size_t)o * side * side;
        for (int i = 0; i < side * side; i++) dst[i] = b[o];

        for (int c = 0; c < in_ch; c++) {
            const float *src = in + (size_t)c * side * side;
            const float *k = w + ((size_t)o * in_ch + c) * K * K;
            for (int ky = 0; ky < K; ky++) {
                for (int kx = 0; kx < K; kx++) {
                    float kv = k[ky * K + kx];
                    if (kv == 0.0f) continue;
                    // input pixel (y+ky-2, x+kx-2) contributes to output (y,x)
                    for (int y = 0; y < side; y++) {
                        int sy = y + ky - K / 2;
                        if (sy < 0 || sy >= side) continue;
                        const float *srow = src + (size_t)sy * side;
                        float *drow = dst + (size_t)y * side;
                        for (int x = 0; x < side; x++) {
                            int sx = x + kx - K / 2;
                            if (sx < 0 || sx >= side) continue;
                            drow[x] += kv * srow[sx];
                        }
                    }
                }
            }
        }
        for (int i = 0; i < side * side; i++)
            if (dst[i] < 0.0f) dst[i] = 0.0f;
    }
}

// Max pool, square kernel, stride == kernel. Truncates a ragged edge, which is
// what ONNX does here: 14 with kernel 3 gives 4, discarding the last two rows.
static void maxpool(const float *in, int ch, int side, int k, float *out)
{
    int oside = (side - k) / k + 1;
    for (int c = 0; c < ch; c++) {
        const float *src = in + (size_t)c * side * side;
        float *dst = out + (size_t)c * oside * oside;
        for (int i = 0; i < oside; i++) {
            for (int j = 0; j < oside; j++) {
                float m = src[(size_t)(i * k) * side + j * k];
                for (int a = 0; a < k; a++)
                    for (int b = 0; b < k; b++) {
                        float v = src[(size_t)(i * k + a) * side + (j * k + b)];
                        if (v > m) m = v;
                    }
                dst[(size_t)i * oside + j] = m;
            }
        }
    }
}

void nn_infer(const float img[28 * 28], nn_result_t *out)
{
    conv_relu(img, 1, 28, conv1_w, conv1_b, (float *)c1_out, C1);
    maxpool((const float *)c1_out, C1, 28, 2, (float *)p1_out);

    conv_relu((const float *)p1_out, C1, 14, conv2_w, conv2_b, (float *)c2_out, C2);
    maxpool((const float *)c2_out, C2, 14, 3, (float *)p2_out);

    // Flattened pool2 is 16*4*4 = 256, ordered channel, row, column. fc_w is
    // stored [256][10] so the flatten order matches without a transpose.
    const float *flat = (const float *)p2_out;
    for (int o = 0; o < 10; o++) {
        float acc = fc_b[o];
        for (int i = 0; i < 256; i++) acc += flat[i] * fc_w[i * 10 + o];
        out->logits[o] = acc;
    }

    float mx = out->logits[0];
    for (int i = 1; i < 10; i++) if (out->logits[i] > mx) mx = out->logits[i];
    float sum = 0.0f;
    for (int i = 0; i < 10; i++) { out->prob[i] = expf(out->logits[i] - mx); sum += out->prob[i]; }
    for (int i = 0; i < 10; i++) out->prob[i] /= sum;

    out->digit = 0;
    for (int i = 1; i < 10; i++) if (out->prob[i] > out->prob[out->digit]) out->digit = i;
    out->confidence = out->prob[out->digit];

    // Hidden row: mean activation of each pair of conv2 channels, scaled so the
    // brightest tap reads full. This is a readout for the LEDs, not part of the
    // classification, so its scaling only has to look right.
    float peak = 0.0f;
    for (int t = 0; t < NN_HIDDEN_TAPS; t++) {
        float acc = 0.0f;
        for (int c = t * 2; c < t * 2 + 2; c++)
            for (int i = 0; i < 16; i++) acc += ((const float *)p2_out)[c * 16 + i];
        acc /= 32.0f;
        out->hidden[t] = acc;
        if (acc > peak) peak = acc;
    }
    if (peak > 0.0f)
        for (int t = 0; t < NN_HIDDEN_TAPS; t++) out->hidden[t] /= peak;
}
