#include "render.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>

// Hardcoded city houses: {x, width, height}
static const float houses[][3] = {
    {  60, 48, 55}, { 130, 36, 45}, { 200, 52, 65}, { 290, 40, 50},
    { 380, 44, 60}, { 460, 38, 48}, { 540, 56, 70}, { 640, 42, 55},
    { 730, 36, 44}, { 810, 50, 62}, { 910, 40, 52}, {1000, 46, 58},
    {1080, 38, 47}, {1140, 44, 53},
};
static const int HOUSE_COUNT = 14;

static void draw_house(float x, float w, float h) {
    // Wall
    DrawRectangle((int)(x - w / 2), GROUND_Y - (int)h,
                  (int)w, (int)h, (Color){160, 160, 160, 255});
    // Roof (triangle)
    Vector2 p1 = {x,             (float)(GROUND_Y - h - 18)};
    Vector2 p2 = {x - w / 2 - 4, (float)(GROUND_Y - h)};
    Vector2 p3 = {x + w / 2 + 4, (float)(GROUND_Y - h)};
    DrawTriangle(p1, p2, p3, (Color){120, 60, 40, 255});
    // Window
    int wx = (int)(x - 7);
    int wy = GROUND_Y - (int)(h * 0.65f);
    DrawRectangle(wx, wy, 14, 12, (Color){200, 220, 255, 255});
}

void render_world(const Agent *agents, int count) {
    // Sky
    DrawRectangle(0, 0, SCREEN_W, WORLD_AREA_H, (Color){135, 185, 220, 255});

    // Houses
    for (int i = 0; i < HOUSE_COUNT; i++) {
        draw_house(houses[i][0], houses[i][1], houses[i][2]);
    }

    // Ground strip
    DrawRectangle(0, GROUND_Y, SCREEN_W, WORLD_AREA_H - GROUND_Y,
                  (Color){80, 140, 60, 255});
    DrawRectangle(0, GROUND_Y, SCREEN_W, 4, (Color){100, 70, 30, 255});

    // Separator line
    DrawRectangle(0, WORLD_AREA_H, SCREEN_W, 2, DARKGRAY);

    // Agents
    for (int i = 0; i < count; i++) {
        const Agent *a = &agents[i];
        float cy = (float)(GROUND_Y - (int)AGENT_RADIUS);

        Color col;
        if (a->tradeFlash > 0.0f) {
            col = YELLOW;
        } else if (a->personalValue > a->expectedMarketValue) {
            col = (Color){60, 200, 80, 255};  // buyer: green
        } else {
            col = (Color){220, 70, 70, 255};  // seller: red
        }

        DrawCircle((int)a->x, (int)cy, AGENT_RADIUS, col);
        DrawCircleLines((int)a->x, (int)cy, AGENT_RADIUS, BLACK);
    }

    // Legend
    DrawCircle(20, 15, 6, (Color){60, 200, 80, 255});
    DrawText("Buyer", 30, 9, 14, BLACK);
    DrawCircle(90, 15, 6, (Color){220, 70, 70, 255});
    DrawText("Seller", 100, 9, 14, BLACK);
    DrawCircle(165, 15, 6, YELLOW);
    DrawText("Trading", 175, 9, 14, BLACK);
}

void render_plot(const PriceHistory *ph, const Agent *agents, int agentCount) {
    int px = PLOT_MARGIN_L;
    int py = WORLD_AREA_H + PLOT_MARGIN_T;
    int pw = SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R;
    int ph_h = SCREEN_H - WORLD_AREA_H - PLOT_MARGIN_T - PLOT_MARGIN_B;

    // Background
    DrawRectangle(0, WORLD_AREA_H + 2, SCREEN_W, SCREEN_H - WORLD_AREA_H - 2,
                  (Color){20, 20, 30, 255});

    // Title
    DrawText("Avg Expected Market Value", px, WORLD_AREA_H + 4, 14,
             (Color){200, 200, 200, 255});

    // Axes
    DrawLine(px, py, px, py + ph_h, LIGHTGRAY);
    DrawLine(px, py + ph_h, px + pw, py + ph_h, LIGHTGRAY);

    // Y-axis labels (0–100 range)
    for (int v = 0; v <= 100; v += 20) {
        int y = py + ph_h - (int)((float)v / 100.0f * (float)ph_h);
        DrawLine(px - 4, y, px, y, LIGHTGRAY);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        DrawText(buf, px - 30, y - 7, 12, LIGHTGRAY);
        // horizontal grid
        DrawLine(px, y, px + pw, y, (Color){50, 50, 60, 255});
    }

    if (ph->count < 2) return;

    // Theoretical equilibrium: median personal value (approx as average)
    float pvSum = 0.0f;
    for (int i = 0; i < agentCount; i++) pvSum += agents[i].personalValue;
    float equilibrium = pvSum / (float)agentCount;
    int eq_y = py + ph_h - (int)(equilibrium / 100.0f * (float)ph_h);
    DrawLine(px, eq_y, px + pw, eq_y, (Color){255, 200, 0, 120});
    DrawText("equilibrium", px + pw - 75, eq_y - 14, 11, (Color){255, 200, 0, 180});

    // Price line
    for (int i = 1; i < ph->count; i++) {
        float v0 = price_history_get(ph, i - 1);
        float v1 = price_history_get(ph, i);
        float t0 = (float)(i - 1) / (float)(PRICE_HISTORY_SIZE - 1);
        float t1 = (float)i        / (float)(PRICE_HISTORY_SIZE - 1);

        int x0 = px + (int)(t0 * (float)pw);
        int x1 = px + (int)(t1 * (float)pw);
        int y0 = py + ph_h - (int)(v0 / 100.0f * (float)ph_h);
        int y1 = py + ph_h - (int)(v1 / 100.0f * (float)ph_h);

        DrawLine(x0, y0, x1, y1, (Color){80, 180, 255, 255});
    }

    // Current value label
    float cur = price_history_get(ph, ph->count - 1);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f", cur);
    int cur_y = py + ph_h - (int)(cur / 100.0f * (float)ph_h);
    DrawText(buf, px + pw + 2, cur_y - 7, 12, (Color){80, 180, 255, 255});
}
