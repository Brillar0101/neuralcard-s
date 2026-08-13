// Air-written digit capture and classification. See infer.h.
#include "infer.h"
#include "canvas.h"
#include "charlieplex.h"
#include "lsm6ds3.h"
#include "nn.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdio.h>

// LSM6DS3 at 245 dps full scale: 8.75 mdps per count.
#define DPS_PER_LSB   0.00875f
#define SAMPLE_HZ     104.0f
#define DT            (1.0f / SAMPLE_HZ)

// Motion gate, degrees per second of total rotation.
#define MOVE_START    60.0f
#define MOVE_STOP     25.0f
#define STILL_SAMPLES 30      // ~300 ms below MOVE_STOP ends the stroke
#define MAX_SAMPLES   400     // ~3.8 s ceiling

// LED rows. D1-D6 are the six input axes, D7-D15 hidden, D16-D24 the digits.
#define ROW_IN     0
#define ROW_HIDDEN 6
#define ROW_OUT    15

static canvas_path_t path;
static float last_img[28 * 28];
static bool  have_img;

static uint8_t lvl(float v)      // 0..1 -> 0..8, with a floor so a lit LED reads
{
    if (v <= 0.01f) return 0;
    int b = (int)(v * 8.0f + 0.5f);
    if (b < 1) b = 1;
    if (b > 8) b = 8;
    return (uint8_t)b;
}

// Bias is whatever the gyro reads while the card is held still, and it changes
// with temperature, so it is measured immediately before every stroke.
static void measure_bias(float *bx, float *by, float *bz)
{
    float sx = 0, sy = 0, sz = 0;
    int n = 0;
    imu_sample_t s;
    for (int i = 0; i < 50; i++) {
        if (imu_data_ready() && imu_read(&s)) {
            sx += s.gx; sy += s.gy; sz += s.gz;
            n++;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (!n) { *bx = *by = *bz = 0; return; }
    *bx = sx / n * DPS_PER_LSB;
    *by = sy / n * DPS_PER_LSB;
    *bz = sz / n * DPS_PER_LSB;
}

int infer_capture_and_classify(void)
{
    float bx, by, bz;
    cp_clear();
    measure_bias(&bx, &by, &bz);

    canvas_begin(&path);
    have_img = false;

    bool writing = false;
    int still = 0, taken = 0;
    imu_sample_t s;

    printf("air-write a digit...\n");

    while (taken < MAX_SAMPLES) {
        if (!imu_data_ready() || !imu_read(&s)) { vTaskDelay(1); continue; }

        float gx = s.gx * DPS_PER_LSB - bx;
        float gy = s.gy * DPS_PER_LSB - by;
        float gz = s.gz * DPS_PER_LSB - bz;
        float mag = sqrtf(gx * gx + gy * gy + gz * gz);

        // Input row mirrors the live axes so the card visibly reacts to motion.
        cp_frame[ROW_IN + 0] = lvl(fabsf((float)s.ax) / 8192.0f);
        cp_frame[ROW_IN + 1] = lvl(fabsf((float)s.ay) / 8192.0f);
        cp_frame[ROW_IN + 2] = lvl(fabsf((float)s.az) / 8192.0f);
        cp_frame[ROW_IN + 3] = lvl(fabsf(gx) / 200.0f);
        cp_frame[ROW_IN + 4] = lvl(fabsf(gy) / 200.0f);
        cp_frame[ROW_IN + 5] = lvl(fabsf(gz) / 200.0f);

        if (!writing) {
            if (mag > MOVE_START) writing = true;
            else continue;
        }

        canvas_add_sample(&path, gx, gy, gz, DT);
        taken++;

        still = (mag < MOVE_STOP) ? still + 1 : 0;
        if (still >= STILL_SAMPLES) break;
    }

    cp_clear();
    if (!canvas_render(&path, last_img)) {
        printf("stroke too small to read\n");
        return -1;
    }
    have_img = true;

    nn_result_t r;
    nn_infer(last_img, &r);

    for (int i = 0; i < NN_HIDDEN_TAPS; i++) cp_frame[ROW_HIDDEN + i] = lvl(r.hidden[i]);
    uint8_t conf[10];
    for (int i = 0; i < 10; i++) conf[i] = (uint8_t)(r.prob[i] * 255.0f);
    cp_set_outputs(conf);

    printf("read %d  (confidence %.0f%%, %d samples)\n",
           r.digit, r.confidence * 100.0f, path.n);
    return r.digit;
}

void infer_dump_last_image(void)
{
    if (!have_img) { printf("no stroke captured yet\n"); return; }
    const char *ramp = " .:-=+*#%@";
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 28; x++) {
            int i = (int)(last_img[y * 28 + x] * 9.0f);
            if (i < 0) i = 0;
            if (i > 9) i = 9;
            putchar(ramp[i]);
        }
        putchar('\n');
    }
}
