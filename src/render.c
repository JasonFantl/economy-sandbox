#include "render.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

static const float houses[][3] = {
    {  60, 48, 55}, { 130, 36, 45}, { 200, 52, 65}, { 290, 40, 50},
    { 380, 44, 60}, { 460, 38, 48}, { 540, 56, 70}, { 640, 42, 55},
    { 730, 36, 44}, { 810, 50, 62}, { 910, 40, 52}, {1000, 46, 58},
    {1080, 38, 47}, {1140, 44, 53},
};
static const int HOUSE_COUNT = 14;

static void draw_house(float x, float w, float h) {
    DrawRectangle((int)(x - w/2), GROUND_Y - (int)h, (int)w, (int)h,
                  (Color){160,160,160,255});
    Vector2 p1 = {x,             (float)(GROUND_Y - h - 18)};
    Vector2 p2 = {x - w/2 - 4,  (float)(GROUND_Y - h)};
    Vector2 p3 = {x + w/2 + 4,  (float)(GROUND_Y - h)};
    DrawTriangle(p1, p2, p3, (Color){120,60,40,255});
    DrawRectangle((int)(x - 7), GROUND_Y - (int)(h * 0.65f), 14, 12,
                  (Color){200,220,255,255});
}

void render_world(const Agent *agents, int count, bool paused, float timeScale) {
    DrawRectangle(0, 0, SCREEN_W, WORLD_AREA_H, (Color){135,185,220,255});

    for (int i = 0; i < HOUSE_COUNT; i++)
        draw_house(houses[i][0], houses[i][1], houses[i][2]);

    DrawRectangle(0, GROUND_Y, SCREEN_W, WORLD_AREA_H - GROUND_Y,
                  (Color){80,140,60,255});
    DrawRectangle(0, GROUND_Y, SCREEN_W, 4, (Color){100,70,30,255});
    DrawRectangle(0, WORLD_AREA_H, SCREEN_W, 2, DARKGRAY);

    for (int i = 0; i < count; i++) {
        const Agent *a = &agents[i];
        float cy = (float)(GROUND_Y - (int)AGENT_RADIUS);
        Color col;
        if (a->tradeFlash > 0.0f)                           col = YELLOW;
        else if (a->personalValue > a->expectedMarketValue) col = (Color){60,200,80,255};
        else                                                 col = (Color){220,70,70,255};
        DrawCircle((int)a->x, (int)cy, AGENT_RADIUS, col);
        DrawCircleLines((int)a->x, (int)cy, AGENT_RADIUS, BLACK);
    }

    // Legend
    DrawCircle(20, 15, 6, (Color){60,200,80,255});
    DrawText("Buyer",   30, 9, 14, BLACK);
    DrawCircle(90, 15, 6, (Color){220,70,70,255});
    DrawText("Seller", 100, 9, 14, BLACK);
    DrawCircle(165, 15, 6, YELLOW);
    DrawText("Trading", 175, 9, 14, BLACK);

    // Speed indicator
    const char *speedStr;
    Color speedCol;
    if (paused)              { speedStr = "PAUSED";    speedCol = RED;    }
    else if (timeScale >= 8) { speedStr = "Speed: 8x"; speedCol = ORANGE; }
    else if (timeScale >= 4) { speedStr = "Speed: 4x"; speedCol = YELLOW; }
    else                     { speedStr = "Speed: 1x"; speedCol = WHITE;  }
    DrawText(speedStr, SCREEN_W - 110, 6, 16, speedCol);
    DrawText("[SPACE] pause  [F] speed", SCREEN_W - 188, 26, 12,
             (Color){40,40,40,255});
}

// ---------------------------------------------------------------------------
// Plot helpers
// ---------------------------------------------------------------------------

static const Agent *s_sort_agents = NULL;
static int cmp_by_personal(const void *a, const void *b) {
    float fa = s_sort_agents[*(const int *)a].personalValue;
    float fb = s_sort_agents[*(const int *)b].personalValue;
    return (fa > fb) - (fa < fb);
}

// Map personal value [20,80] to a blue→red color (semi-transparent)
static Color agent_color(float personalValue, unsigned char alpha) {
    float t = (personalValue - 20.0f) / 60.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (Color){
        (unsigned char)(55 + t * 200),
        30,
        (unsigned char)(255 - t * 200),
        alpha
    };
}

static void draw_axes(int px, int py, int pw, int ph,
                       float equilibrium, const char *title) {
    DrawText(title, px, WORLD_AREA_H + 4, 13, (Color){200,200,200,255});
    DrawLine(px, py, px, py + ph, LIGHTGRAY);
    DrawLine(px, py + ph, px + pw, py + ph, LIGHTGRAY);
    for (int v = 0; v <= 100; v += 20) {
        int y = py + ph - (int)((float)v / 100.0f * (float)ph);
        DrawLine(px - 4, y, px, y, LIGHTGRAY);
        char buf[8]; snprintf(buf, sizeof(buf), "%d", v);
        DrawText(buf, px - 28, y - 7, 11, LIGHTGRAY);
        DrawLine(px, y, px + pw, y, (Color){50,50,60,255});
    }
    if (equilibrium > 0.0f) {
        int eq_y = py + ph - (int)(equilibrium / 100.0f * (float)ph);
        DrawLine(px, eq_y, px + pw, eq_y, (Color){255,200,0,110});
        DrawText("equil.", px + pw - 46, eq_y - 13, 11,
                 (Color){255,200,0,180});
    }
}

// ---------------------------------------------------------------------------
// Left panel: current agent values (sorted by personalValue)
// ---------------------------------------------------------------------------

static void draw_agent_panel(const Agent *agents, int count,
                              int px, int py, int pw, int ph,
                              float equilibrium) {
    draw_axes(px, py, pw, ph, equilibrium,
              "Agent Values (sorted by personal value)");

    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    s_sort_agents = agents;
    qsort(indices, (size_t)count, sizeof(int), cmp_by_personal);

    float xStep = (float)pw / (float)(count - 1);
    for (int rank = 0; rank < count; rank++) {
        const Agent *a  = &agents[indices[rank]];
        int x           = px + (int)((float)rank * xStep);
        int y_pv        = py + ph - (int)(a->personalValue       / 100.0f * (float)ph);
        int y_emv       = py + ph - (int)(a->expectedMarketValue / 100.0f * (float)ph);

        DrawLine(x, y_pv, x, y_emv, (Color){100,100,100,160});
        DrawCircle(x, y_pv, 3, (Color){80,140,255,220});

        Color emvCol = (a->tradeFlash > 0.0f)
                       ? YELLOW
                       : (a->personalValue > a->expectedMarketValue
                          ? (Color){60,210,90,220}
                          : (Color){220,70,70,220});
        DrawCircle(x, y_emv, 3, emvCol);
    }

    int lx = px + pw - 135, ly = py + 5;
    DrawCircle(lx, ly + 4, 3, (Color){80,140,255,220});
    DrawText("Personal val.",  lx + 7, ly - 1,  11, (Color){80,140,255,220});
    DrawCircle(lx, ly + 18, 3, (Color){60,210,90,220});
    DrawText("Expected val.", lx + 7, ly + 13, 11, (Color){60,210,90,220});
}

// ---------------------------------------------------------------------------
// Right panel: per-agent expected value over time
// ---------------------------------------------------------------------------

static void draw_timeseries_panel(const AgentValueHistory *avh,
                                   const Agent *agents,
                                   int px, int py, int pw, int ph,
                                   float equilibrium) {
    draw_axes(px, py, pw, ph, equilibrium,
              "Expected Market Values (each agent, over time)");

    if (avh->count < 2) return;

    // One thin line per agent, colored by personalValue
    for (int ag = 0; ag < avh->agentCount; ag++) {
        Color col = agent_color(agents[ag].personalValue, 55);
        float xScale = (float)pw / (float)(PRICE_HISTORY_SIZE - 1);
        for (int s = 1; s < avh->count; s++) {
            float v0 = avh_get(avh, ag, s - 1);
            float v1 = avh_get(avh, ag, s);
            float t0 = (float)(s - 1) * xScale;
            float t1 = (float)s       * xScale;
            int x0   = px + (int)t0;
            int x1   = px + (int)t1;
            int y0   = py + ph - (int)(v0 / 100.0f * (float)ph);
            int y1   = py + ph - (int)(v1 / 100.0f * (float)ph);
            DrawLine(x0, y0, x1, y1, col);
        }
    }

    // Average line on top (bright white)
    for (int s = 1; s < avh->count; s++) {
        float v0 = avh_avg(avh, s - 1);
        float v1 = avh_avg(avh, s);
        float xScale = (float)pw / (float)(PRICE_HISTORY_SIZE - 1);
        int x0 = px + (int)((float)(s-1) * xScale);
        int x1 = px + (int)((float)s     * xScale);
        int y0 = py + ph - (int)(v0 / 100.0f * (float)ph);
        int y1 = py + ph - (int)(v1 / 100.0f * (float)ph);
        DrawLine(x0, y0, x1, y1, WHITE);
    }

    // Legend
    int lx = px + pw - 95, ly = py + 5;
    DrawLine(lx, ly + 4, lx + 18, ly + 4, agent_color(50.0f, 180));
    DrawText("Agent",   lx + 22, ly - 1,  11, (Color){200,200,200,255});
    DrawLine(lx, ly + 18, lx + 18, ly + 18, WHITE);
    DrawText("Average", lx + 22, ly + 13, 11, WHITE);
}

// ---------------------------------------------------------------------------
// Top-level
// ---------------------------------------------------------------------------

void render_plot(const AgentValueHistory *avh, const Agent *agents,
                 int agentCount) {
    DrawRectangle(0, WORLD_AREA_H + 2, SCREEN_W, SCREEN_H - WORLD_AREA_H - 2,
                  (Color){20,20,30,255});

    int py      = WORLD_AREA_H + PLOT_MARGIN_T + 15;
    int pheight = SCREEN_H - WORLD_AREA_H - PLOT_MARGIN_T - PLOT_MARGIN_B - 15;
    int half    = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int px_left  = PLOT_MARGIN_L;
    int px_right = PLOT_MARGIN_L + half + PANEL_GAP;

    float pvSum = 0.0f;
    for (int i = 0; i < agentCount; i++) pvSum += agents[i].personalValue;
    float equilibrium = pvSum / (float)agentCount;

    draw_agent_panel(agents, agentCount, px_left, py, half, pheight, equilibrium);

    DrawLine(px_right - PANEL_GAP/2, WORLD_AREA_H + 4,
             px_right - PANEL_GAP/2, SCREEN_H - 4,
             (Color){60,60,70,255});

    draw_timeseries_panel(avh, agents, px_right, py, half, pheight, equilibrium);
}
