#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
#include <math.h>
#include <stdbool.h>

#define NUM_AGENTS 120
#define PRICE_RECORD_INTERVAL 0.25f  // simulation seconds between price samples

int main(void) {
    SetRandomSeed(42);

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    // --- Init simulation ---
    Agent agents[NUM_AGENTS];
    agents_init(agents, NUM_AGENTS, WORLD_WIDTH);

    PriceHistory priceHistory = {0};
    float priceTimer = 0.0f;

    bool  paused    = false;
    float timeScale = 1.0f;  // simulation speed multiplier

    while (!WindowShouldClose()) {
        // --- Input ---
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_F)) {
            // Cycle: 1x → 4x → 8x → 1x
            if (timeScale < 2.0f)       timeScale = 4.0f;
            else if (timeScale < 6.0f)  timeScale = 8.0f;
            else                        timeScale = 1.0f;
        }

        float dt = paused ? 0.0f : GetFrameTime() * timeScale;

        // --- Update ---
        agents_update(agents, NUM_AGENTS, dt);

        for (int i = 0; i < NUM_AGENTS; i++) {
            Agent *a = &agents[i];
            Agent *b = &agents[a->targetId];
            if (fabsf(a->x - b->x) < AGENT_RADIUS * 2.0f) {
                market_trade(a, b);
                a->targetId = agents_pick_target(NUM_AGENTS, i);
                b->targetId = agents_pick_target(NUM_AGENTS, a->targetId);
            }
        }

        priceTimer += dt;
        if (priceTimer >= PRICE_RECORD_INTERVAL) {
            price_history_record(&priceHistory, agents, NUM_AGENTS);
            priceTimer = 0.0f;
        }

        // --- Draw ---
        BeginDrawing();
        ClearBackground(BLACK);

        render_world(agents, NUM_AGENTS, paused, timeScale);
        render_plot(&priceHistory, agents, NUM_AGENTS);

        DrawFPS(4, 4);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
