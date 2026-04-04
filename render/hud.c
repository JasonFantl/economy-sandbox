#include "render/hud.h"
#include "render/camera.h"
#include "render/input.h"
#include "econ/econ.h"
#include "raylib.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Panel state
// ---------------------------------------------------------------------------

static PanelState s_panels[NUM_PANELS] = {
    { PLOT_WEALTH,                 MARKET_WOOD  },
    { PLOT_PRICE_HISTORY,          MARKET_WOOD  },
    { PLOT_VALUATION_DISTRIBUTION, MARKET_WOOD  },
    { PLOT_PRICE_HISTORY,          MARKET_CHAIR },
};

static Inspector s_inspector;

InfluencePanel g_influence;
DecayRatePanel g_decay_rates;
Assets         g_assets;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void hud_init(void) {
    inspector_init(&s_inspector);
    influence_panel_init(&g_influence);
    decay_rate_panel_init(&g_decay_rates);
    assets_load(&g_assets);
}

void hud_unload(void) {
    assets_unload(&g_assets);
}

// ---------------------------------------------------------------------------
// Free-play frame: input then render
// ---------------------------------------------------------------------------

void hud_freeplay_frame(void) {
    // Input
    input_handle_speed();
    if (IsKeyPressed(KEY_SPACE)) g_simulation.paused = !g_simulation.paused;
    panel_handle_bounds_keyboard();
    bool consumed = panel_handle_click(s_panels);
    if (!consumed) consumed = influence_panel_update(&g_influence, g_simulation.agents, g_simulation.count);
    if (!consumed) consumed = decay_rate_panel_update(&g_decay_rates);
    if (!consumed) inspector_update(&s_inspector, g_simulation.agents, g_simulation.count,
                                    g_camX, g_camY, g_camZoom);

    // Render
    render_panels_freeplay(g_simulation.avh, g_simulation.pvh, g_simulation.gvh,
                           g_simulation.agents, g_simulation.count, s_panels);
    influence_panel_render(&g_influence);
    decay_rate_panel_render(&g_decay_rates);
    inspector_render(&s_inspector, g_simulation.agents, g_camX, g_camY, g_camZoom);
}

// ---------------------------------------------------------------------------
// Walkthrough frame: input then render
// ---------------------------------------------------------------------------

void hud_walkthrough_frame(WalkthroughState *wt, SimContext *ctx) {
    // Input
    input_handle_speed();
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

    // Render
    int plotY = g_world_view_y + WORLD_VIEW_H + 2;
    int plotH = SCREEN_H - plotY;
    DrawRectangle(0, plotY, SCREEN_W, plotH, (Color){20, 20, 30, 255});
    const StepDef *step = walkthrough_current_step(wt);
    if (step->render_panels)
        step->render_panels(ctx, PLOT_MARGIN_L, plotY,
                            SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R,
                            plotH - PLOT_MARGIN_B);
    walkthrough_render_overlay(wt);
}
