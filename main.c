#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
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
                market_trade(a, b);
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
                          int simSteps) {
    BeginDrawing();
    ClearBackground(BLACK);
    render_world(agents, count, paused, simSteps);
    render_plot(&avh, agents, count);
    DrawFPS(4, 4);
    EndDrawing();
}

int main(void) {
    SetRandomSeed(42);

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    Agent agents[NUM_AGENTS];
    agents_init(agents, NUM_AGENTS, WORLD_WIDTH);

    bool paused   = false;
    int  simSteps = 1;  // update calls per frame (1 = normal, 4 = fast, 8 = faster)

    while (!WindowShouldClose()) {
        // Input
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_F)) {
            if      (simSteps == 1) simSteps = 4;
            else if (simSteps == 4) simSteps = 8;
            else                    simSteps = 1;
        }

        // Update
        if (!paused) {
            float dt = GetFrameTime();
            for (int s = 0; s < simSteps; s++) {
                simulation_step(agents, NUM_AGENTS, dt);
            }
        }

        // Render
        render_frame(agents, NUM_AGENTS, paused, simSteps);
    }

    CloseWindow();
    return 0;
}
