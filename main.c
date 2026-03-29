#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
#include "src/inspector.h"
#include "src/controls.h"
#include "src/assets.h"
#include <math.h>
#include <stdbool.h>

#define NUM_AGENTS            120
#define PRICE_RECORD_INTERVAL 0.25f

// History arrays indexed by market — static so they live in BSS, not the stack
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
            if (fabsf(a->body.x - b->body.x) < AGENT_RADIUS * 2.0f) {
                // Gossip and trade for every market on each encounter
                for (int mid = 0; mid < MARKET_COUNT; mid++) {
                    MarketId m = (MarketId)mid;
                    market_gossip(a, b, m);
                    if (market_is_buyer(AGENT_MKT(a, m), a->econ.money) && market_is_seller(AGENT_MKT(b, m), b->econ.money))
                        market_trade(a, b, m);
                    else if (market_is_buyer(AGENT_MKT(b, m), b->econ.money) && market_is_seller(AGENT_MKT(a, m), a->econ.money))
                        market_trade(b, a, m);
                }

                agents_pick_new_target(a, count, WORLD_WIDTH);
                agents_pick_new_target(b, count, WORLD_WIDTH);
            }
        } else {
            if (fabsf(a->body.x - a->body.targetX) < AGENT_RADIUS * 2.0f) {
                agents_pick_new_target(a, count, WORLD_WIDTH);
            }
        }
    }

    priceTimer += dt;
    if (priceTimer >= PRICE_RECORD_INTERVAL) {
        for (int mid = 0; mid < MARKET_COUNT; mid++) {
            MarketId m = (MarketId)mid;
            avh_record(&avh[mid], agents, NUM_AGENTS, m);
            avh_record_personal(&pvh[mid], agents, NUM_AGENTS, m);
            avh_record_goods(&gvh[mid], agents, NUM_AGENTS, m);
        }
        priceTimer = 0.0f;
    }
}

static void render_frame(const Agent *agents, int count, bool paused,
                          int simSteps, const Inspector *ins,
                          const InfluencePanel *inf, const BreakRatePanel *br,
                          const Assets *assets,
                          PanelState panels[NUM_PANELS]) {
    BeginDrawing();
    ClearBackground(BLACK);
    render_world(agents, count, paused, simSteps, assets);
    render_plot(avh, pvh, gvh, agents, count, panels);
    influence_panel_render(inf);
    break_rate_panel_render(br);
    inspector_render(ins, agents);
    DrawFPS(4, 4);
    EndDrawing();
}

int main(void) {
    SetRandomSeed(42);

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    Agent agents[NUM_AGENTS];
    agents_init(agents, NUM_AGENTS, WORLD_WIDTH);

    bool       paused   = false;
    int        simSteps = 1;
    PanelState panels[NUM_PANELS] = {
        { PLOT_WEALTH,       MARKET_WOOD  },   // top-left
        { PLOT_EMV_HISTORY,  MARKET_WOOD  },   // top-right
        { PLOT_AGENT_VALUES, MARKET_WOOD  },   // bottom-left
        { PLOT_EMV_HISTORY,  MARKET_CHAIR },   // bottom-right
    };
    Inspector      inspector;
    InfluencePanel influence;
    BreakRatePanel breakRates;
    Assets         assets;
    inspector_init(&inspector);
    influence_panel_init(&influence);
    break_rate_panel_init(&breakRates);
    assets_load(&assets);

    while (!WindowShouldClose()) {
        // Simulation speed controls
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_F)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (simSteps > 1) simSteps /= 2;
            } else {
                simSteps *= 2;
            }
        }

        // UI input (plot strips → influence panel → inspector, in z-order)
        panel_handle_bounds_keyboard();
        bool consumed = panel_handle_click(panels);
        if (!consumed) consumed = influence_panel_update(&influence, agents, NUM_AGENTS);
        if (!consumed) consumed = break_rate_panel_update(&breakRates);
        if (!consumed) inspector_update(&inspector, agents, NUM_AGENTS);

        // Update
        if (!paused) {
            float dt = GetFrameTime();
            for (int s = 0; s < simSteps; s++) {
                simulation_step(agents, NUM_AGENTS, dt);
            }
        }

        render_frame(agents, NUM_AGENTS, paused, simSteps, &inspector, &influence, &breakRates, &assets,
                     panels);
    }

    assets_unload(&assets);
    CloseWindow();
    return 0;
}
