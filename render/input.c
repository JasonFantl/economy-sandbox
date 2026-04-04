#include "render/input.h"
#include "sim.h"
#include "raylib.h"

void input_handle_pause(void) {
    if (IsKeyPressed(KEY_SPACE)) g_simulation.paused = !g_simulation.paused;
}

void input_handle_speed(void) {
    if (!IsKeyPressed(KEY_F)) return;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        if (g_simulation.ticks_per_frame > 1) g_simulation.ticks_per_frame /= 2;
    } else {
        g_simulation.ticks_per_frame *= 2;
    }
}
