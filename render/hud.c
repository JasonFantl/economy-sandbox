#include "render/hud.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/input.h"
#include "econ/econ.h"
#include "raygui.h"
#include "raylib.h"
#include <string.h>

#define FONT_PATH "assets/fonts/Silkscreen-Regular.ttf"
#define FONT_BASE_SIZE 8

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
    GuiLoadStyleDefault();
    g_font = LoadFontEx(FONT_PATH, FONT_BASE_SIZE, 0, 250);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_POINT);
    GuiSetFont(g_font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    inspector_init(&s_inspector);
    influence_panel_init(&g_influence);
    decay_rate_panel_init(&g_decay_rates);
    assets_load(&g_assets);
}

void hud_unload(void) {
    UnloadFont(g_font);
    assets_unload(&g_assets);
}

// ---------------------------------------------------------------------------
// Free-play frame: input then render (raygui panels handle their own input)
// ---------------------------------------------------------------------------

void hud_freeplay_frame(void) {
    input_handle_speed();
    input_handle_pause();
    panel_handle_bounds_keyboard();
    bool consumed = panel_handle_click(s_panels);
    if (!consumed) inspector_update(&s_inspector, g_simulation.agents, g_simulation.count,
                                    g_camX, g_camY, g_camZoom);

    render_panels_freeplay(g_simulation.avh, g_simulation.pvh, g_simulation.gvh,
                           g_simulation.agents, g_simulation.count, s_panels);
    influence_panel_render(&g_influence, g_simulation.agents, g_simulation.count);
    decay_rate_panel_render(&g_decay_rates);
    inspector_render(&s_inspector, g_simulation.agents, g_camX, g_camY, g_camZoom);
}

// ---------------------------------------------------------------------------
// Walkthrough frame: input then render
// ---------------------------------------------------------------------------

void hud_walkthrough_frame(WalkthroughState *wt, SimContext *ctx) {
    input_handle_speed();
    input_handle_pause();
    bool consumed = walkthrough_handle_input(wt, ctx);
    if (consumed) {
        memset(g_simulation.avh, 0, sizeof(g_simulation.avh));
        memset(g_simulation.pvh, 0, sizeof(g_simulation.pvh));
        memset(g_simulation.gvh, 0, sizeof(g_simulation.gvh));
        g_simulation.priceTick = 0;
    }

    int plotY = g_world_view_y + WORLD_VIEW_H + 2;
    int plotH = SCREEN_H - plotY;
    DrawRectangle(0, plotY, SCREEN_W, plotH, (Color){20, 20, 30, 255});
    const StepDef *step = walkthrough_current_step(wt);
    if (step->render_panels)
        step->render_panels(ctx, PLOT_MARGIN_L, plotY,
                            SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R,
                            plotH - PLOT_MARGIN_B);
    walkthrough_render_overlay(wt, ctx);
}
