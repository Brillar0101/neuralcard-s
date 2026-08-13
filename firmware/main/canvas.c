// Gesture to MNIST-shaped bitmap. See canvas.h.
#include "canvas.h"
#include <math.h>
#include <string.h>

// Running angular displacement, degrees, reset by canvas_begin().
static float ang_yaw, ang_pitch;

void canvas_begin(canvas_path_t *p)
{
    ang_yaw = ang_pitch = 0.0f;
    p->n = 0;
}

void canvas_add_sample(canvas_path_t *p, float gx, float gy, float gz, float dt)
{
    (void)gx;  // roll is the axis you twist along, it carries no pen position

    // Yaw sweeps the pen left and right, pitch sweeps it up and down. Screen y
    // grows downward, so pitch is negated to keep a written digit upright.
    ang_yaw   += gz * dt;
    ang_pitch += gy * dt;

    if (p->n >= CANVAS_MAX_POINTS) return;
    p->x[p->n] =  ang_yaw;
    p->y[p->n] = -ang_pitch;
    p->n++;
}

// Paint a disc of radius r at (cx, cy), taking the max so overlapping strokes
// do not accumulate past 1. MNIST strokes are soft edged, so this is smooth
// rather than a hard circle.
static void splat(float img[28 * 28], float cx, float cy, float r)
{
    int x0 = (int)floorf(cx - r), x1 = (int)ceilf(cx + r);
    int y0 = (int)floorf(cy - r), y1 = (int)ceilf(cy + r);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > 27) x1 = 27;
    if (y1 > 27) y1 = 27;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx, dy = (float)y - cy;
            float d = sqrtf(dx * dx + dy * dy);
            float v = 1.0f - (d - r * 0.35f) / (r * 0.9f);   // 1 in the core, fading out
            if (v <= 0.0f) continue;
            if (v > 1.0f) v = 1.0f;
            float *px = &img[y * 28 + x];
            if (v > *px) *px = v;
        }
    }
}

int canvas_render(const canvas_path_t *p, float img[28 * 28])
{
    memset(img, 0, sizeof(float) * 28 * 28);
    if (p->n < 8) return 0;

    float minx = p->x[0], maxx = p->x[0], miny = p->y[0], maxy = p->y[0];
    for (int i = 1; i < p->n; i++) {
        if (p->x[i] < minx) minx = p->x[i];
        if (p->x[i] > maxx) maxx = p->x[i];
        if (p->y[i] < miny) miny = p->y[i];
        if (p->y[i] > maxy) maxy = p->y[i];
    }
    float w = maxx - minx, h = maxy - miny;
    float span = w > h ? w : h;
    if (span < 5.0f) return 0;        // under 5 degrees of sweep is a twitch, not a digit

    // Fit the longer side into 20 px, preserving aspect, then centre the
    // bounding box in the 28x28 field. A second pass re-centres on the ink's
    // centre of mass, which is how MNIST itself is framed.
    float s = 20.0f / span;
    float ox = 14.0f - (minx + w * 0.5f) * s;
    float oy = 14.0f - (miny + h * 0.5f) * s;

    float stroke = 1.2f;
    for (int i = 1; i < p->n; i++) {
        float ax = p->x[i - 1] * s + ox, ay = p->y[i - 1] * s + oy;
        float bx = p->x[i] * s + ox,     by = p->y[i] * s + oy;
        float dx = bx - ax, dy = by - ay;
        float len = sqrtf(dx * dx + dy * dy);
        int steps = (int)(len * 2.0f) + 1;      // half pixel spacing, no gaps
        for (int k = 0; k <= steps; k++) {
            float t = (float)k / (float)steps;
            splat(img, ax + dx * t, ay + dy * t, stroke);
        }
    }

    // Re-centre by centre of mass, the way MNIST digits are positioned.
    float sum = 0.0f, cx = 0.0f, cy = 0.0f;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 28; x++) {
            float v = img[y * 28 + x];
            sum += v; cx += v * x; cy += v * y;
        }
    if (sum <= 0.0f) return 0;
    cx /= sum; cy /= sum;

    int sx = (int)lrintf(13.5f - cx), sy = (int)lrintf(13.5f - cy);
    if (sx || sy) {
        static float tmp[28 * 28];
        memcpy(tmp, img, sizeof(tmp));
        memset(img, 0, sizeof(float) * 28 * 28);
        for (int y = 0; y < 28; y++) {
            int ty = y + sy;
            if (ty < 0 || ty > 27) continue;
            for (int x = 0; x < 28; x++) {
                int tx = x + sx;
                if (tx < 0 || tx > 27) continue;
                img[ty * 28 + tx] = tmp[y * 28 + x];
            }
        }
    }
    return 1;
}
