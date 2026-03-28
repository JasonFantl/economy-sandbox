#include "render.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Hardcoded city houses: {x, width, height}
static const float houses[][3] = {
    {  60, 48, 55}, { 130, 36, 45}, { 200, 52, 65}, { 290, 40, 50},
    { 380, 44, 60}, { 460, 38, 48}, { 540, 56, 70}, { 640, 42, 55},
    { 730, 36, 44}, { 810, 50, 62}, { 910, 40, 52}, {1000, 46, 58},
    {1080, 38, 47}, {1140, 44, 53},
};
static const int HOUSE_COUNT = 14;

static void draw_house(float x, float w, float h) {
    DrawRectangle((int)(x - w / 2), GROUND_Y - (int)h,
                  (int)w, (int)h, (Color){160, 160, 160, 255});
    Vector2 p1 = {x,             (float)(GROUND_Y - h - 18)};
    Vector2 p2 = {x - w / 2 - 4, (float)(GROUND_Y - h)};
    Vector2 p3 = {x + w / 2 + 4, (float)(GROUND_Y - h)};
    DrawTriangle(p1, p2, p3, (Color){120, 60, 40, 255});
    DrawRectangle((int)(x - 7), GROUND_Y - (int)(h * 0.65f), 14, 12,
                  (Color){200, 220, 255, 255});
}

void render_world(const Agent *agents, int count, bool paused, float timeScale) {
    DrawRectangle(0, 0, SCREEN_W, WORLD_AREA_H, (Color){135, 185, 220, 255});

    for (int i = 0; i < HOUSE_COUNT; i++) {
        draw_house(houses[i][0], houses[i][1], houses[i][2]);
    }

    DrawRectangle(0, GROUND_Y, SCREEN_W, WORLD_AREA_H - GROUND_Y,
                  (Color){80, 140, 60, 255});
    DrawRectangle(0, GROUND_Y, SCREEN_W, 4, (Color){100, 70, 30, 255});
    DrawRectangle(0, WORLD_AREA_H, SCREEN_W, 2, DARKGRAY);

    for (int i = 0; i < count; i++) {
        const Agent *a = &agents[i];
        float cy = (float)(GROUND_Y - (int)AGENT_RADIUS);

        Color col;
        if (a->tradeFlash > 0.0f) {
            col = YELLOW;
        } else if (a->personalValue > a->expectedMarketValue) {
            col = (Color){60, 200, 80, 255};
        } else {
            col = (Color){220, 70, 70, 255};
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

    // Controls / speed indicator (top right)
    const char *speedStr;
    Color speedCol;
    if (paused) {
        speedStr = "PAUSED";
        speedCol = RED;
    } else if (timeScale >= 8.0f) {
        speedStr = "Speed: 8x";
        speedCol = ORANGE;
    } else if (timeScale >= 4.0f) {
        speedStr = "Speed: 4x";
        speedCol = YELLOW;
    } else {
        speedStr = "Speed: 1x";
        speedCol = WHITE;
    }
    DrawText(speedStr, SCREEN_W - 110, 6, 16, speedCol);
    DrawText("[SPACE] pause  [F] speed", SCREEN_W - 188, 26, 12,
             (Color){40, 40, 40, 255});
}

// ---- Agent dot plot helpers ----

static const Agent *s_sort_agents = NULL;
static int cmp_by_personal(const void *a, const void *b) {
    float fa = s_sort_agents[*(const int *)a].personalValue;
    float fb = s_sort_agents[*(const int *)b].personalValue;
    return (fa > fb) - (fa < fb);
}

// Draw one sub-panel: axes, grid, equilibrium line.
// Returns the inner draw area rect via out params.
static void draw_panel_axes(int px, int py, int pw, int ph,
                             float equilibrium, const char *title) {
    DrawText(title, px, WORLD_AREA_H + 4, 13, (Color){200, 200, 200, 255});

    DrawLine(px, py, px, py + ph, LIGHTGRAY);
    DrawLine(px, py + ph, px + pw, py + ph, LIGHTGRAY);

    for (int v = 0; v <= 100; v += 20) {
        int y = py + ph - (int)((float)v / 100.0f * (float)ph);
        DrawLine(px - 4, y, px, y, LIGHTGRAY);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        DrawText(buf, px - 28, y - 7, 11, LIGHTGRAY);
        DrawLine(px, y, px + pw, y, (Color){50, 50, 60, 255});
    }

    if (equilibrium > 0.0f) {
        int eq_y = py + ph - (int)(equilibrium / 100.0f * (float)ph);
        DrawLine(px, eq_y, px + pw, eq_y, (Color){255, 200, 0, 110});
    }
}

static void draw_agent_panel(const Agent *agents, int count,
                              int px, int py, int pw, int ph,
                              float equilibrium) {
    draw_panel_axes(px, py, pw, ph, equilibrium, "Agent Values (sorted by personal value)");

    // Equilibrium label
    if (equilibrium > 0.0f) {
        int eq_y = py + ph - (int)(equilibrium / 100.0f * (float)ph);
        DrawText("equil.", px + pw - 46, eq_y - 13, 11, (Color){255, 200, 0, 180});
    }

    // Sort agent indices by personalValue
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    s_sort_agents = agents;
    qsort(indices, (size_t)count, sizeof(int), cmp_by_personal);

    float xStep = (float)pw / (float)(count - 1);

    for (int rank = 0; rank < count; rank++) {
        const Agent *a = &agents[indices[rank]];
        int x = px + (int)((float)rank * xStep);

        int py_pv  = py + ph - (int)(a->personalValue       / 100.0f * (float)ph);
        int py_emv = py + ph - (int)(a->expectedMarketValue / 100.0f * (float)ph);

        // Connector line between personal and expected
        DrawLine(x, py_pv, x, py_emv, (Color){100, 100, 100, 180});

        // Personal value: blue
        DrawCircle(x, py_pv, 3, (Color){80, 140, 255, 220});

        // Expected market value: green (buyer) / red (seller)
        Color emvCol = (a->personalValue > a->expectedMarketValue)
                       ? (Color){60, 210, 90, 220}
                       : (Color){220, 70, 70, 220};
        if (a->tradeFlash > 0.0f) emvCol = YELLOW;
        DrawCircle(x, py_emv, 3, emvCol);
    }

    // Legend inside panel
    int lx = px + pw - 130, ly = py + 5;
    DrawCircle(lx,      ly + 4, 3, (Color){80, 140, 255, 220});
    DrawText("Personal val.", lx + 7, ly - 1, 11, (Color){80, 140, 255, 220});
    DrawCircle(lx, ly + 18, 3, (Color){60, 210, 90, 220});
    DrawText("Expected val.", lx + 7, ly + 13, 11, (Color){60, 210, 90, 220});
}

static void draw_timeseries_panel(const PriceHistory *ph,
                                   int px, int py, int pw, int ph_h,
                                   float equilibrium) {
    draw_panel_axes(px, py, pw, ph_h, equilibrium, "Avg Expected Market Value (over time)");

    if (equilibrium > 0.0f) {
        int eq_y = py + ph_h - (int)(equilibrium / 100.0f * (float)ph_h);
        DrawText("equil.", px + pw - 46, eq_y - 13, 11, (Color){255, 200, 0, 180});
    }

    if (ph->count < 2) return;

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

    float cur = price_history_get(ph, ph->count - 1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", cur);
    int cur_y = py + ph_h - (int)(cur / 100.0f * (float)ph_h);
    DrawText(buf, px + pw + 2, cur_y - 7, 12, (Color){80, 180, 255, 255});
}

void render_plot(const PriceHistory *ph, const Agent *agents, int agentCount) {
    // Background
    DrawRectangle(0, WORLD_AREA_H + 2, SCREEN_W, SCREEN_H - WORLD_AREA_H - 2,
                  (Color){20, 20, 30, 255});

    // Shared Y geometry
    int py      = WORLD_AREA_H + PLOT_MARGIN_T + 15; // extra for title
    int pheight = SCREEN_H - WORLD_AREA_H - PLOT_MARGIN_T - PLOT_MARGIN_B - 15;
    int half    = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;

    int px_left  = PLOT_MARGIN_L;
    int px_right = PLOT_MARGIN_L + half + PANEL_GAP;

    // Equilibrium estimate (avg personal value)
    float pvSum = 0.0f;
    for (int i = 0; i < agentCount; i++) pvSum += agents[i].personalValue;
    float equilibrium = pvSum / (float)agentCount;

    draw_agent_panel(agents, agentCount, px_left, py, half, pheight, equilibrium);

    // Divider
    DrawLine(px_right - PANEL_GAP / 2, WORLD_AREA_H + 4,
             px_right - PANEL_GAP / 2, SCREEN_H - 4,
             (Color){60, 60, 70, 255});

    draw_timeseries_panel(ph, px_right, py, half, pheight, equilibrium);
}
