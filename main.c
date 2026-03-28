#include "raylib.h"
#include "src/agent.h"
#include "src/market.h"
#include "src/render.h"
#include <math.h>

#define NUM_AGENTS 120
#define PRICE_RECORD_INTERVAL 0.3f  // seconds between price samples

int main(void) {
    SetRandomSeed(42);

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox");
    SetTargetFPS(60);

    // --- Init simulation ---
    Agent agents[NUM_AGENTS];
    agents_init(agents, NUM_AGENTS, WORLD_WIDTH);

    PriceHistory priceHistory = {0};
    float priceTimer = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- Update ---
        agents_update(agents, NUM_AGENTS, dt);

        // Check if any agent reached its target → attempt trade, pick new target
        for (int i = 0; i < NUM_AGENTS; i++) {
            Agent *a = &agents[i];
            Agent *b = &agents[a->targetId];
            if (fabsf(a->x - b->x) < AGENT_RADIUS * 2.0f) {
                market_trade(a, b);
                a->targetId = agents_pick_target(NUM_AGENTS, i);
                b->targetId = agents_pick_target(NUM_AGENTS, a->targetId);
            }
        }

        // Record price history at intervals
        priceTimer += dt;
        if (priceTimer >= PRICE_RECORD_INTERVAL) {
            price_history_record(&priceHistory, agents, NUM_AGENTS);
            priceTimer = 0.0f;
        }

        // --- Draw ---
        BeginDrawing();
        ClearBackground(BLACK);

        render_world(agents, NUM_AGENTS);
        render_plot(&priceHistory, agents, NUM_AGENTS);

        DrawFPS(SCREEN_W - 80, 4);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
