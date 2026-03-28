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
#define PRICE_RECORD_INTERVAL 0.25f  // simulation-seconds between samples

// 320KB history array — static so it lives in BSS, not the stack
static AgentValueHistory avh = {0};

static float priceTimer = 0.0f;

static void simulation_step(Agent *agents, int count, float dt) {
    agents_update(agents, count, dt);

    for (int i = 0; i < count; i++) {
        Agent *a = &agents[i];
        if (a->targetType == TARGET_AGENT) {
            Agent *b = &agents[a->targetId];
            if (fabsf(a->x - b->x) < AGENT_RADIUS * 2.0f) {
                // Gossip on every encounter
                market_gossip(a, b, 0.5);

                // Trade only if one is a buyer and the other is a seller
                if (agent_is_buyer(a) && agent_is_seller(b))       market_trade(a, b);
                else if (agent_is_buyer(b) && agent_is_seller(a))  market_trade(b, a);

                agents_pick_new_target(a, count, WORLD_WIDTH);
                agents_pick_new_target(b, count, WORLD_WIDTH);
            }
        } else {
            if (fabsf(a->x - a->targetX) < AGENT_RADIUS * 2.0f) {
                agents_pick_new_target(a, count, WORLD_WIDTH);
            }
        }
    }

    priceTimer += dt;
    if (priceTimer >= PRICE_RECORD_INTERVAL) {
        avh_record(&avh, agents, NUM_AGENTS);
        priceTimer = 0.0f;
    }
}

static void render_frame(const Agent *agents, int count, bool paused,
                          int simSteps, const Inspector *ins,
                          const InfluencePanel *inf, const Assets *assets) {
    BeginDrawing();
    ClearBackground(BLACK);
    render_world(agents, count, paused, simSteps, assets);
    render_plot(&avh, agents, count);
    influence_panel_render(inf);
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

    bool          paused    = false;
    int           simSteps  = 1;
    Inspector     inspector;
    InfluencePanel influence;
    Assets        assets;
    inspector_init(&inspector);
    influence_panel_init(&influence);
    assets_load(&assets);

    while (!WindowShouldClose()) {
        // Input: simulation controls
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_F)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (simSteps > 1) simSteps /= 2;
            } else {
                simSteps *= 2;
            }
        }

        // Input: UI panels (influence panel checked first; inspector gets remaining clicks)
        bool consumed = influence_panel_update(&influence, agents, NUM_AGENTS);
        if (!consumed) inspector_update(&inspector, agents, NUM_AGENTS);

        // Update
        if (!paused) {
            float dt = GetFrameTime();
            for (int s = 0; s < simSteps; s++) {
                simulation_step(agents, NUM_AGENTS, dt);
            }
        }

        // Render
        render_frame(agents, NUM_AGENTS, paused, simSteps, &inspector, &influence, &assets);
    }

    assets_unload(&assets);
    CloseWindow();
    return 0;
}
