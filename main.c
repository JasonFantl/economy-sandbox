#include "raylib.h"
#include "sim.h"
#include "render/camera.h"
#include "render/input.h"
#include "render/render.h"
#include "render/inspector.h"
#include "render/controls.h"
#include "world/world.h"
#include "world/tileset.h"
#include "walkthrough/walkthrough.h"
#include "econ/nav.h"

#define MAP_FILE "map.emap"

int main(void) {
    SetRandomSeed(42);
    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    WorldMap *map = worldmap_load(MAP_FILE);
    TileAtlas tiles = {0};
    tileatlas_load(&tiles);

    g_simulation.worldW = 1200.0f;
    g_simulation.worldH = 400.0f;
    if (map) {
        g_simulation.worldW = (float)(map->width  * TILE_SIZE) * WORLD_TILE_SCALE;
        g_simulation.worldH = (float)(map->height * TILE_SIZE) * WORLD_TILE_SCALE;
        if (g_simulation.worldH > (float)WORLD_VIEW_H) g_simulation.worldH = (float)WORLD_VIEW_H;
    }
    camera_init();

    agents_nav_init(map);
    g_simulation.count = NUM_AGENTS;
    agents_init(g_simulation.agents, g_simulation.count, g_simulation.worldW, g_simulation.worldH);

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

    SimContext ctx = { .sim = &g_simulation, .inf = &influence, .decay = &decayRates };
    WalkthroughState wt;
    walkthrough_init(&wt, &ctx);

    while (!WindowShouldClose()) {
        g_world_view_y = wt.active ? WTHROUGH_NAV_H : 0;

        input_handle_speed();
        camera_update(g_world_view_y);

        if (wt.active)
            input_handle_walkthrough(&wt, &ctx);
        else
            input_handle_freeplay(panels, &influence, &decayRates, &inspector);

        sim_update();

        BeginDrawing();
        ClearBackground(BLACK);
        render_world(map, &tiles, g_simulation.agents, g_simulation.count,
                     g_simulation.paused, g_simulation.ticks_per_frame, &assets,
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
            render_panels_freeplay(g_simulation.avh, g_simulation.pvh, g_simulation.gvh,
                                   g_simulation.agents, g_simulation.count, panels);
            influence_panel_render(&influence);
            decay_rate_panel_render(&decayRates);
            inspector_render(&inspector, g_simulation.agents, g_camX, g_camY, g_camZoom);
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
