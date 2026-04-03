#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
#include "src/inspector.h"
#include "src/controls.h"
#include "src/assets.h"
#include "src/world.h"
#include "src/tileset.h"
#include "src/walkthrough.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define NUM_AGENTS            120
#define PRICE_RECORD_INTERVAL 0.25f
#define MAP_FILE              "map.emap"

// World dimensions in pixels (derived from tile map or fallback constants)
static float g_world_w = 1200.0f;
static float g_world_h = 400.0f;

// Camera — world point at viewport centre, and zoom level
static float g_camX   = 0.0f;
static float g_camY   = 0.0f;
static float g_camZoom = 1.0f;
#define CAM_ZOOM_MIN 0.25f
#define CAM_ZOOM_MAX 6.0f

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
            int bi = a->body.targetId;
            Agent *b = &agents[bi];
            float dx = a->body.x - b->body.x;
            float dy = a->body.y - b->body.y;
            if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
                for (int mid = 0; mid < MARKET_COUNT; mid++) {
                    MarketId m = (MarketId)mid;
                    market_gossip(a, b, m);
                    if (wants_to_buy(AGENT_MKT(a, m), a->econ.money) && wants_to_sell(AGENT_MKT(b, m), b->econ.money))
                        market_trade(a, b, m);
                    else if (wants_to_buy(AGENT_MKT(b, m), b->econ.money) && wants_to_sell(AGENT_MKT(a, m), a->econ.money))
                        market_trade(b, a, m);
                }
                agents_pick_new_target(agents, i,  count, g_world_w, g_world_h);
                agents_pick_new_target(agents, bi, count, g_world_w, g_world_h);
            }
        } else if (a->body.targetType == TARGET_WORK_CHOP ||
                   a->body.targetType == TARGET_WORK_BUILD) {
            float dx = a->body.x - a->body.targetX;
            float dy = a->body.y - a->body.targetY;
            if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
                if (a->body.targetType == TARGET_WORK_CHOP)
                    agent_execute_chop(a);
                else
                    agent_execute_build(a);
                agents_pick_new_target(agents, i, count, g_world_w, g_world_h);
            }
        } else {  // TARGET_POS
            float dx = a->body.x - a->body.targetX;
            float dy = a->body.y - a->body.targetY;
            if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
                agents_pick_new_target(agents, i, count, g_world_w, g_world_h);
            }
        }
    }

    priceTimer += dt;
    if (priceTimer >= PRICE_RECORD_INTERVAL) {
        for (int mid = 0; mid < MARKET_COUNT; mid++) {
            MarketId m = (MarketId)mid;
            avh_record_prices(&avh[mid], agents, count, m);
            avh_record_personal_valuations(&pvh[mid], agents, count, m);
            avh_record_goods(&gvh[mid], agents, count, m);
        }
        priceTimer = 0.0f;
    }
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
        if (g_world_h > (float)WORLD_VIEW_H) g_world_h = (float)WORLD_VIEW_H;
    }
    // Start camera so world top-left appears at viewport top-left
    g_camX = (float)SCREEN_W   * 0.5f;
    g_camY = (float)WORLD_VIEW_H * 0.5f;

    // Build walkable nav graph (must happen before agents_init)
    agents_nav_init(map);

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

    // Walkthrough setup
    SimContext ctx;
    ctx.agents = agents;
    ctx.count  = NUM_AGENTS;
    ctx.avh    = avh;
    ctx.pvh    = pvh;
    ctx.gvh    = gvh;
    ctx.inf    = &influence;
    ctx.decay  = &decayRates;
    ctx.worldW = g_world_w;
    ctx.worldH = g_world_h;

    WalkthroughState wt;
    walkthrough_init(&wt, &ctx);

    while (!WindowShouldClose()) {
        // Determine active agent count
        int activeCount = wt.active ? ctx.count : NUM_AGENTS;

        if (wt.active) {
            g_world_view_y = WTHROUGH_NAV_H;
            bool consumed = walkthrough_handle_input(&wt, &ctx);
            if (consumed) {
                // Agent count may have changed; reset history
                activeCount = ctx.count;
                priceTimer = 0.0f;
                memset(avh, 0, sizeof(avh));
                memset(pvh, 0, sizeof(pvh));
                memset(gvh, 0, sizeof(gvh));
            }
            // In walkthrough mode, allow decay/influence panel input if shown
            if (!consumed) {
                const StepDef *step = walkthrough_current_step(&wt);
                if (step->decayEnabled)
                    decay_rate_panel_update(&decayRates);
                if (step->productionEnabled || step->leisureEnabled)
                    influence_panel_update(&influence, agents, activeCount);
            }
        } else {
            g_world_view_y = 0;

            if (IsKeyPressed(KEY_SPACE)) paused = !paused;
            if (IsKeyPressed(KEY_F)) {
                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                    if (simSteps > 1) simSteps /= 2;
                } else {
                    simSteps *= 2;
                }
            }

            // Camera pan (middle-mouse drag) and zoom (scroll wheel)
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

            panel_handle_bounds_keyboard();
            bool consumed = panel_handle_click(panels);
            if (!consumed) consumed = influence_panel_update(&influence, agents, activeCount);
            if (!consumed) consumed = decay_rate_panel_update(&decayRates);
            if (!consumed) inspector_update(&inspector, agents, activeCount,
                                            g_camX, g_camY, g_camZoom);
        }

        if (!paused) {
            float dt = GetFrameTime();
            for (int s = 0; s < simSteps; s++)
                simulation_step(agents, activeCount, dt);
        }

        // Render
        BeginDrawing();
        ClearBackground(BLACK);
        render_world(map, &tiles, agents, activeCount, paused, simSteps, &assets,
                     g_camX, g_camY, g_camZoom);

        if (wt.active) {
            // Walkthrough plot area
            int plotY = g_world_view_y + WORLD_VIEW_H + 2;
            int plotH = SCREEN_H - plotY;
            DrawRectangle(0, plotY, SCREEN_W, plotH, (Color){20, 20, 30, 255});

            const StepDef *step = walkthrough_current_step(&wt);
            if (step->render_panels) {
                step->render_panels(&ctx,
                                    PLOT_MARGIN_L, plotY,
                                    SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R,
                                    plotH - PLOT_MARGIN_B);
            }
            walkthrough_render_overlay(&wt);
        } else {
            render_panels_freeplay(avh, pvh, gvh, agents, activeCount, panels);
            influence_panel_render(&influence);
            decay_rate_panel_render(&decayRates);
            inspector_render(&inspector, agents, g_camX, g_camY, g_camZoom);
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
