#include "render.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

// Sprite display scale (32px cell → 64px on screen)
#define SPRITE_SCALE 2.0f
#define SPRITE_DISP  (SPRITE_FRAME_SIZE * SPRITE_SCALE)  // 64px

// ---- Village decorations ---------------------------------------------------

typedef struct {
    Rectangle src;    // source rect in village tile sheet
    float     x;      // screen x centre
    float     scale;
} Decor;

// Buildings drawn behind agents; trees drawn in front (foreground depth)
static const Decor BUILDINGS[] = {
    {{  6, 216, 162, 168},  80, 0.60f},  // red-roof house
    {{195,  24, 218, 168}, 270, 0.58f},  // wide double house
    {{427,  24, 106, 168}, 450, 0.62f},  // shop/inn
    {{ 10,  24, 162, 168}, 620, 0.60f},  // blue house
    {{555, 233, 161, 151}, 800, 0.62f},  // tall house variant
    {{746,  62,  76, 130}, 940, 0.68f},  // small tower
    {{423, 216, 106, 168},1060, 0.62f},  // shop variant
    {{559,  41, 161, 151},1170, 0.60f},  // tall house
};
static const int BUILDING_COUNT = 8;

static const Decor TREES[] = {
    {{423, 438, 84, 106},  30, 0.72f},
    {{519, 438, 84, 106}, 185, 0.72f},
    {{615, 438, 84, 106}, 370, 0.72f},
    {{ 34, 451, 60, 116}, 545, 0.72f},  // shrubs
    {{711, 438, 84, 106}, 710, 0.72f},
    {{423, 438, 84, 106}, 880, 0.72f},
    {{615, 438, 84, 106},1030, 0.72f},
    {{519, 438, 84, 106},1155, 0.72f},
};
static const int TREE_COUNT = 8;

static void draw_decor(const Decor *d, Texture2D sheet) {
    float dw = d->src.width  * d->scale;
    float dh = d->src.height * d->scale;
    Rectangle dst = {d->x - dw / 2.0f, (float)GROUND_Y - dh, dw, dh};
    DrawTexturePro(sheet, d->src, dst, (Vector2){0, 0}, 0.0f, WHITE);
}

static void draw_agent(const Agent *a, const Assets *assets) {
    // Source rect — negative width flips horizontally (east-facing)
    float fw = a->facingRight
               ? (float)SPRITE_FRAME_SIZE
               : -(float)SPRITE_FRAME_SIZE;
    float fx = a->facingRight
               ? (float)(a->animFrame * SPRITE_FRAME_SIZE)
               : (float)((a->animFrame + 1) * SPRITE_FRAME_SIZE); // flip origin

    Rectangle src = {
        fx,
        (float)(SPRITE_WALK_ROW * SPRITE_FRAME_SIZE),
        fw,
        (float)SPRITE_FRAME_SIZE
    };

    // Destination: bottom of sprite sits on GROUND_Y, centered on agent.x
    Rectangle dst = {
        a->x - SPRITE_DISP / 2.0f,
        (float)GROUND_Y - SPRITE_DISP,
        SPRITE_DISP,
        SPRITE_DISP
    };

    // Tint: yellow flash when trading, otherwise white (no tint)
    Color tint = (a->tradeFlash > 0.0f) ? YELLOW : WHITE;
    DrawTexturePro(assets->sprites[a->spriteType], src, dst,
                   (Vector2){0, 0}, 0.0f, tint);

    // Small status dot at agent feet (buyer=green, seller=red)
    Color dot = (a->personalValue > a->expectedMarketValue)
                ? (Color){60, 210, 90, 200}
                : (Color){220, 70, 70, 200};
    DrawCircle((int)a->x, GROUND_Y + 3, 3, dot);
}

void render_world(const Agent *agents, int count, bool paused, int simSteps,
                  const Assets *assets) {
    // Background stretched to fill world area
    DrawTexturePro(
        assets->background,
        (Rectangle){0, 0, (float)assets->background.width,
                          (float)assets->background.height},
        (Rectangle){0, 0, (float)SCREEN_W, (float)WORLD_AREA_H},
        (Vector2){0, 0}, 0.0f, WHITE
    );

    // Buildings (behind agents)
    for (int i = 0; i < BUILDING_COUNT; i++)
        draw_decor(&BUILDINGS[i], assets->village);

    // Agents (mid-ground)
    for (int i = 0; i < count; i++)
        draw_agent(&agents[i], assets);

    // Trees (foreground — drawn last so they overlap agents for depth)
    for (int i = 0; i < TREE_COUNT; i++)
        draw_decor(&TREES[i], assets->village);

    // Separator line between world and plot
    DrawRectangle(0, WORLD_AREA_H, SCREEN_W, 2, DARKGRAY);

    // Legend (semi-transparent backing so it reads over any background)
    DrawRectangle(0, 0, 270, 28, (Color){0, 0, 0, 100});
    DrawCircle(10, 14, 5, (Color){60,210,90,255});
    DrawText("Buyer",   20, 7, 14, WHITE);
    DrawCircle(75, 14, 5, (Color){220,70,70,255});
    DrawText("Seller",  85, 7, 14, WHITE);
    DrawCircle(145, 14, 5, YELLOW);
    DrawText("Trading", 155, 7, 14, WHITE);

    // Speed indicator
    char speedBuf[32];
    Color speedCol;
    if (paused)            { snprintf(speedBuf, sizeof(speedBuf), "PAUSED");               speedCol = RED;    }
    else if (simSteps > 1) { snprintf(speedBuf, sizeof(speedBuf), "Speed: %dx", simSteps); speedCol = ORANGE; }
    else                   { snprintf(speedBuf, sizeof(speedBuf), "Speed: 1x");            speedCol = WHITE;  }
    DrawRectangle(SCREEN_W - 200, 0, 200, 42, (Color){0, 0, 0, 100});
    DrawText(speedBuf, SCREEN_W - 110, 6, 16, speedCol);
    DrawText("[SPACE] pause  [F] speed", SCREEN_W - 188, 26, 12, (Color){200,200,200,255});
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
