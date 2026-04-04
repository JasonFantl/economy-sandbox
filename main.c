#include "raylib.h"
#include "sim.h"
#include "render/camera.h"
#include "render/input.h"
#include "render/hud.h"
#include "render/render.h"
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
    hud_init();

    agents_nav_init(map);
    g_simulation.count = NUM_AGENTS;
    agents_init(g_simulation.agents, g_simulation.count, g_simulation.worldW, g_simulation.worldH);

    SimContext ctx = { .sim = &g_simulation, .inf = &g_influence, .decay = &g_decay_rates };
    WalkthroughState wt;
    walkthrough_init(&wt, &ctx);

    while (!WindowShouldClose()) {
        g_world_view_y = wt.active ? WTHROUGH_NAV_H : 0;
        input_handle_speed();
        camera_update(g_world_view_y);
        sim_update();

        BeginDrawing();
        ClearBackground(BLACK);
        render_world(map, &tiles, g_simulation.agents, g_simulation.count,
                     g_simulation.paused, g_simulation.ticks_per_frame, &g_assets,
                     g_camX, g_camY, g_camZoom);

        if (wt.active)
            hud_walkthrough_frame(&wt, &ctx);
        else
            hud_freeplay_frame();

        DrawFPS(4, wt.active ? WTHROUGH_NAV_H + 4 : 4);
        EndDrawing();
    }

    tileatlas_unload(&tiles);
    if (map) worldmap_free(map);
    hud_unload();
    CloseWindow();
    return 0;
}
