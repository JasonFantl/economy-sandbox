#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
#include <math.h>
#include <stdbool.h>

#define NUM_AGENTS 120
#define PRICE_RECORD_INTERVAL 0.25f  // simulation-seconds between samples

// Static so the 320KB history array lives in BSS, not on the stack
static AgentValueHistory avh = {0};

int main(void) {
    SetRandomSeed(42);

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    Agent agents[NUM_AGENTS];
    agents_init(agents, NUM_AGENTS, WORLD_WIDTH);

    float priceTimer = 0.0f;
    bool  paused     = false;
    float timeScale  = 1.0f;

    while (!WindowShouldClose()) {
        // Input
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_F)) {
            if      (timeScale < 2.0f) timeScale = 4.0f;
            else if (timeScale < 6.0f) timeScale = 8.0f;
            else                       timeScale = 1.0f;
        }

        float dt = paused ? 0.0f : GetFrameTime() * timeScale;

        // Update
        agents_update(agents, NUM_AGENTS, dt);

        for (int i = 0; i < NUM_AGENTS; i++) {
            Agent *a = &agents[i];
            if (a->targetType == TARGET_AGENT) {
                Agent *b = &agents[a->targetId];
                if (fabsf(a->x - b->x) < AGENT_RADIUS * 2.0f) {
                    market_trade(a, b);
                    agents_pick_new_target(a, NUM_AGENTS, WORLD_WIDTH);
                    agents_pick_new_target(b, NUM_AGENTS, WORLD_WIDTH);
                }
            } else { // TARGET_POS
                if (fabsf(a->x - a->targetX) < AGENT_RADIUS * 2.0f) {
                    agents_pick_new_target(a, NUM_AGENTS, WORLD_WIDTH);
                }
            }
        }

        priceTimer += dt;
        if (priceTimer >= PRICE_RECORD_INTERVAL) {
            avh_record(&avh, agents, NUM_AGENTS);
            priceTimer = 0.0f;
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);
        render_world(agents, NUM_AGENTS, paused, timeScale);
        render_plot(&avh, agents, NUM_AGENTS);
        DrawFPS(4, 4);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
