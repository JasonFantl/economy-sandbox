#include "raylib.h"
#include "sim.h"
#include "econ/nav.h"
#include "render/render.h"
#include "render/inspector.h"
#include "render/controls.h"
#include "world/world.h"
#include "world/tileset.h"
#include "walkthrough/walkthrough.h"
#include <math.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Sim state (authoritative source for all simulation data)
// ---------------------------------------------------------------------------
SimState g_sim = { .paused = false, .steps = 1 };

#define PRICE_RECORD_TICKS 15   // record prices every 15 ticks (0.25s at 60 ticks/s)
#define MAP_FILE           "map.emap"

// ---------------------------------------------------------------------------
// Camera (viewport state, not sim state)
// ---------------------------------------------------------------------------
static float g_camX    = 0.0f;
static float g_camY    = 0.0f;
static float g_camZoom = 1.0f;

#define CAM_ZOOM_MIN 0.25f
#define CAM_ZOOM_MAX 6.0f

// ---------------------------------------------------------------------------
// Sim update
// ---------------------------------------------------------------------------
static void sim_step(void) {
    agents_update(g_sim.agents, g_sim.count);

    for (int i = 0; i < g_sim.count; i++)
        agent_attempt_trade(g_sim.agents, i, g_sim.count, g_sim.worldW, g_sim.worldH);

    if (++g_sim.priceTick >= PRICE_RECORD_TICKS) {
        for (int mid = 0; mid < MARKET_COUNT; mid++) {
            MarketId m = (MarketId)mid;
            avh_record_prices(&g_sim.avh[mid], g_sim.agents, g_sim.count, m);
            avh_record_personal_valuations(&g_sim.pvh[mid], g_sim.agents, g_sim.count, m);
            avh_record_goods(&g_sim.gvh[mid], g_sim.agents, g_sim.count, m);
        }
        g_sim.priceTick = 0;
    }
}

static void sim_update(void) {
    if (g_sim.paused) return;
    for (int s = 0; s < g_sim.steps; s++)
        sim_step();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    SetRandomSeed(42);
    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    WorldMap *map = worldmap_load(MAP_FILE);
    TileAtlas tiles = {0};
    tileatlas_load(&tiles);

    g_sim.worldW = 1200.0f;
    g_sim.worldH = 400.0f;
    if (map) {
        g_sim.worldW = (float)(map->width  * TILE_SIZE) * WORLD_TILE_SCALE;
        g_sim.worldH = (float)(map->height * TILE_SIZE) * WORLD_TILE_SCALE;
        if (g_sim.worldH > (float)WORLD_VIEW_H) g_sim.worldH = (float)WORLD_VIEW_H;
    }
    g_camX = (float)SCREEN_W    * 0.5f;
    g_camY = (float)WORLD_VIEW_H * 0.5f;

    agents_nav_init(map);
    g_sim.count = NUM_AGENTS;
    agents_init(g_sim.agents, g_sim.count, g_sim.worldW, g_sim.worldH);

    PanelState panels[NUM_PANELS] = {
        { PLOT_WEALTH,                 MARKET_WOOD  },
        { PLOT_PRICE_HISTORY,          MARKET_WOOD  },
        { PLOT_VALUATION_DISTRIBUTION, MARKET_WOOD  },
        { PLOT_PRICE_HISTORY,          MARKET_CHAIR },
    };
    Inspector      inspector;
    InfluencePanel influence;
    DecayRatePanel decayRates;
    Assets         assets;
    inspector_init(&inspector);
    influence_panel_init(&influence);
    decay_rate_panel_init(&decayRates);
    assets_load(&assets);

    SimContext ctx = { .sim = &g_sim, .inf = &influence, .decay = &decayRates };
    WalkthroughState wt;
    walkthrough_init(&wt, &ctx);

    while (!WindowShouldClose()) {

        // --- Input ---
        if (IsKeyPressed(KEY_F)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (g_sim.steps > 1) g_sim.steps /= 2;
            } else {
                g_sim.steps *= 2;
            }
        }

        g_world_view_y = wt.active ? WTHROUGH_NAV_H : 0;

        // --- Camera pan (middle-mouse drag) and zoom — always active ---
        {
            Vector2 mouse = GetMousePosition();
            bool inWorld = mouse.y >= g_world_view_y && mouse.y <= g_world_view_y + (float)WORLD_VIEW_H;

            if (inWorld && IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
                Vector2 delta = GetMouseDelta();
                g_camX -= delta.x / g_camZoom;
                g_camY -= delta.y / g_camZoom;
            }

            if (inWorld) {
                float wheel = GetMouseWheelMove();
                if (wheel != 0.0f) {
                    float preWx = (mouse.x - (float)SCREEN_W * 0.5f) / g_camZoom + g_camX;
                    float preWy = (mouse.y - ((float)g_world_view_y + (float)WORLD_VIEW_H * 0.5f)) / g_camZoom + g_camY;
                    float factor = (wheel > 0.0f) ? 1.1f : 1.0f / 1.1f;
                    g_camZoom *= factor;
                    if (g_camZoom < CAM_ZOOM_MIN) g_camZoom = CAM_ZOOM_MIN;
                    if (g_camZoom > CAM_ZOOM_MAX) g_camZoom = CAM_ZOOM_MAX;
                    g_camX = preWx - (mouse.x - (float)SCREEN_W * 0.5f) / g_camZoom;
                    g_camY = preWy - (mouse.y - ((float)g_world_view_y + (float)WORLD_VIEW_H * 0.5f)) / g_camZoom;
                }
            }
        }

        if (wt.active) {
            bool consumed = walkthrough_handle_input(&wt, &ctx);
            if (consumed) {
                memset(g_sim.avh, 0, sizeof(g_sim.avh));
                memset(g_sim.pvh, 0, sizeof(g_sim.pvh));
                memset(g_sim.gvh, 0, sizeof(g_sim.gvh));
                g_sim.priceTick = 0;
            }
            if (!consumed) {
                if (g_wood_decay_rate > 0.0f || g_chair_decay_rate > 0.0f)
                    decay_rate_panel_update(&decayRates);
                if (g_production_enabled || g_leisure_enabled)
                    influence_panel_update(&influence, g_sim.agents, g_sim.count);
            }
        } else {
            if (IsKeyPressed(KEY_SPACE)) g_sim.paused = !g_sim.paused;
            panel_handle_bounds_keyboard();
            bool consumed = panel_handle_click(panels);
            if (!consumed) consumed = influence_panel_update(&influence, g_sim.agents, g_sim.count);
            if (!consumed) consumed = decay_rate_panel_update(&decayRates);
            if (!consumed) inspector_update(&inspector, g_sim.agents, g_sim.count,
                                            g_camX, g_camY, g_camZoom);
        }

        // --- Simulate ---
        sim_update();

        // --- Render ---
        BeginDrawing();
        ClearBackground(BLACK);
        render_world(map, &tiles, g_sim.agents, g_sim.count, g_sim.paused, g_sim.steps, &assets,
                     g_camX, g_camY, g_camZoom);

        if (wt.active) {
            int plotY = g_world_view_y + WORLD_VIEW_H + 2;
            int plotH = SCREEN_H - plotY;
            DrawRectangle(0, plotY, SCREEN_W, plotH, (Color){20, 20, 30, 255});
            const StepDef *step = walkthrough_current_step(&wt);
            if (step->render_panels)
                step->render_panels(&ctx, PLOT_MARGIN_L, plotY,
                                    SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R,
                                    plotH - PLOT_MARGIN_B);
            walkthrough_render_overlay(&wt);
        } else {
            render_panels_freeplay(g_sim.avh, g_sim.pvh, g_sim.gvh,
                                   g_sim.agents, g_sim.count, panels);
            influence_panel_render(&influence);
            decay_rate_panel_render(&decayRates);
            inspector_render(&inspector, g_sim.agents, g_camX, g_camY, g_camZoom);
        }

        DrawFPS(4, wt.active ? WTHROUGH_NAV_H + 4 : 4);
        EndDrawing();
    }

    tileatlas_unload(&tiles);
    if (map) worldmap_free(map);
    assets_unload(&assets);
    CloseWindow();
    return 0;
}
