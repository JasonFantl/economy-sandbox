#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
#include "src/inspector.h"
#include "src/controls.h"
#include "src/assets.h"
#include "src/world.h"
#include "src/tileset.h"
#include <math.h>
#include <stdbool.h>

#define NUM_AGENTS            120
#define PRICE_RECORD_INTERVAL 0.25f
#define MAP_FILE              "map.emap"

// World dimensions in pixels (derived from tile map or fallback constants)
static float g_world_w = 1200.0f;
static float g_world_h = 400.0f;

// History arrays indexed by market
static AgentValueHistory avh[MARKET_COUNT];
static AgentValueHistory pvh[MARKET_COUNT];
static AgentValueHistory gvh[MARKET_COUNT];

static float priceTimer = 0.0f;

static void simulation_step(Agent *agents, int count, float dt) {
    agents_update(agents, count, dt);

    for (int i = 0; i < count; i++) {
        Agent *a = &agents[i];
        if (a->body.targetType == TARGET_AGENT) {
            Agent *b = &agents[a->body.targetId];
            float dx = a->body.x - b->body.x;
            float dy = a->body.y - b->body.y;
            float dist2 = dx*dx + dy*dy;
            if (dist2 < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
                for (int mid = 0; mid < MARKET_COUNT; mid++) {
                    MarketId m = (MarketId)mid;
                    market_gossip(a, b, m);
                    if (wants_to_buy(AGENT_MKT(a, m), a->econ.money) && wants_to_sell(AGENT_MKT(b, m), b->econ.money))
                        market_trade(a, b, m);
                    else if (wants_to_buy(AGENT_MKT(b, m), b->econ.money) && wants_to_sell(AGENT_MKT(a, m), a->econ.money))
                        market_trade(b, a, m);
                }
                agents_pick_new_target(a, count, g_world_w, g_world_h);
                agents_pick_new_target(b, count, g_world_w, g_world_h);
            }
        } else {
            float dx = a->body.x - a->body.targetX;
            float dy = a->body.y - a->body.targetY;
            if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
                agents_pick_new_target(a, count, g_world_w, g_world_h);
            }
        }
    }

    priceTimer += dt;
    if (priceTimer >= PRICE_RECORD_INTERVAL) {
        for (int mid = 0; mid < MARKET_COUNT; mid++) {
            MarketId m = (MarketId)mid;
            avh_record_prices(&avh[mid], agents, NUM_AGENTS, m);
            avh_record_personal_valuations(&pvh[mid], agents, NUM_AGENTS, m);
            avh_record_goods(&gvh[mid], agents, NUM_AGENTS, m);
        }
        priceTimer = 0.0f;
    }
}

static void render_frame(const WorldMap *map, const TileAtlas *tiles,
                          const Agent *agents, int count, bool paused,
                          int simSteps, const Inspector *ins,
                          const InfluencePanel *inf, const DecayRatePanel *decay,
                          const Assets *assets,
                          PanelState panels[NUM_PANELS]) {
    BeginDrawing();
    ClearBackground(BLACK);
    render_world(map, tiles, agents, count, paused, simSteps, assets);
    render_plot(avh, pvh, gvh, agents, count, panels);
    influence_panel_render(inf);
    decay_rate_panel_render(decay);
    inspector_render(ins, agents);
    DrawFPS(4, 4);
    EndDrawing();
}

int main(void) {
    SetRandomSeed(42);

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    // Load tile map (optional — game runs without one)
    WorldMap *map = worldmap_load(MAP_FILE);
    TileAtlas tiles = {0};
    tileatlas_load(&tiles);

    if (map) {
        g_world_w = (float)(map->width  * TILE_SIZE) * WORLD_TILE_SCALE;
        g_world_h = (float)(map->height * TILE_SIZE) * WORLD_TILE_SCALE;
        // Clamp world height to the visible world viewport
        if (g_world_h > (float)WORLD_VIEW_H) g_world_h = (float)WORLD_VIEW_H;
    }

    Agent agents[NUM_AGENTS];
    agents_init(agents, NUM_AGENTS, g_world_w, g_world_h);

    bool       paused   = false;
    int        simSteps = 1;
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

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_F)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (simSteps > 1) simSteps /= 2;
            } else {
                simSteps *= 2;
            }
        }

        panel_handle_bounds_keyboard();
        bool consumed = panel_handle_click(panels);
        if (!consumed) consumed = influence_panel_update(&influence, agents, NUM_AGENTS);
        if (!consumed) consumed = decay_rate_panel_update(&decayRates);
        if (!consumed) inspector_update(&inspector, agents, NUM_AGENTS);

        if (!paused) {
            float dt = GetFrameTime();
            for (int s = 0; s < simSteps; s++)
                simulation_step(agents, NUM_AGENTS, dt);
        }

        render_frame(map, &tiles, agents, NUM_AGENTS, paused, simSteps,
                     &inspector, &influence, &decayRates, &assets, panels);
    }

    tileatlas_unload(&tiles);
    if (map) worldmap_free(map);
    assets_unload(&assets);
    CloseWindow();
    return 0;
}
