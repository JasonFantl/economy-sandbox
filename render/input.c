#include "render/input.h"
#include "econ/econ.h"
#include "raylib.h"
#include <string.h>

void input_handle_speed(void) {
    if (!IsKeyPressed(KEY_F)) return;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        if (g_simulation.ticks_per_frame > 1) g_simulation.ticks_per_frame /= 2;
    } else {
        g_simulation.ticks_per_frame *= 2;
    }
}

bool input_handle_walkthrough(WalkthroughState *wt, SimContext *ctx) {
    bool consumed = walkthrough_handle_input(wt, ctx);
    if (consumed) {
        memset(g_simulation.avh, 0, sizeof(g_simulation.avh));
        memset(g_simulation.pvh, 0, sizeof(g_simulation.pvh));
        memset(g_simulation.gvh, 0, sizeof(g_simulation.gvh));
        g_simulation.priceTick = 0;
    }
    if (!consumed) {
        if (g_wood_decay_rate > 0.0f || g_chair_decay_rate > 0.0f)
            decay_rate_panel_update(ctx->decay);
        if (g_production_enabled || g_leisure_enabled)
            influence_panel_update(ctx->inf, g_simulation.agents, g_simulation.count);
    }
    return consumed;
}

void input_handle_freeplay(PanelState panels[NUM_PANELS], InfluencePanel *inf,
                            DecayRatePanel *decay, Inspector *inspector) {
    if (IsKeyPressed(KEY_SPACE)) g_simulation.paused = !g_simulation.paused;
    panel_handle_bounds_keyboard();
    bool consumed = panel_handle_click(panels);
    if (!consumed) consumed = influence_panel_update(inf, g_simulation.agents, g_simulation.count);
    if (!consumed) consumed = decay_rate_panel_update(decay);
    if (!consumed) inspector_update(inspector, g_simulation.agents, g_simulation.count,
                                    g_camX, g_camY, g_camZoom);
}
