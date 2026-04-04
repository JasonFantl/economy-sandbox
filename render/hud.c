#include "render/hud.h"
#include "render/render.h"
#include "render/camera.h"
#include "render/input.h"
#include "econ/econ.h"
#include "sim.h"
#include "raygui.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

#define FONT_PATH "assets/fonts/romulus.png"

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
    g_font = LoadFont(FONT_PATH);
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
// World-viewport HUD overlay: legend and speed indicator
// ---------------------------------------------------------------------------

static void render_world_hud(void) {
    // Legend
    DrawRectangle(0, g_world_view_y, 440, 26, (Color){0,0,0,120});
    DrawCircle( 10, g_world_view_y+13, 4,(Color){150,150,150,255}); DrawTextF("Leisure",  18, g_world_view_y+6, 13, WHITE);
    DrawCircle( 90, g_world_view_y+13, 4,(Color){160,100, 40,255}); DrawTextF("Chopping", 98, g_world_view_y+6, 13, WHITE);
    DrawCircle(190, g_world_view_y+13, 4,(Color){220,140, 60,255}); DrawTextF("Building",198, g_world_view_y+6, 13, WHITE);
    DrawCircle(285, g_world_view_y+13, 4, YELLOW);                  DrawTextF("Trading", 293, g_world_view_y+6, 13, WHITE);

    // Speed indicator
    char speedBuf[32]; Color speedCol;
    if      (g_simulation.paused)                 { snprintf(speedBuf,sizeof(speedBuf),"PAUSED");                          speedCol=RED;    }
    else if (g_simulation.ticks_per_frame > 1)    { snprintf(speedBuf,sizeof(speedBuf),"Speed: %dx",g_simulation.ticks_per_frame); speedCol=ORANGE; }
    else                                          { snprintf(speedBuf,sizeof(speedBuf),"Speed: 1x");                       speedCol=WHITE;  }
    DrawRectangle(SCREEN_W-200, g_world_view_y, 200, 40, (Color){0,0,0,100});
    DrawTextF(speedBuf,     SCREEN_W-108, g_world_view_y+4,  16, speedCol);
    DrawTextF("[SPACE]/[F]",SCREEN_W-186, g_world_view_y+24, 11, (Color){200,200,200,255});
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

    render_world_hud();
    DrawRectangle(0, g_world_view_y + WORLD_VIEW_H, SCREEN_W, 2, DARKGRAY);
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

    render_world_hud();

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
