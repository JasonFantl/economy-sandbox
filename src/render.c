#include "render.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

#define SPRITE_DISP  (SPRITE_FRAME_SIZE * SPRITE_SCALE)  // 64px

// ---- Village decorations ---------------------------------------------------

typedef struct {
    int   houseIdx;   // index into assets->houses[]
    float x;          // screen x centre
    float scale;
} HouseDecor;

static const HouseDecor HOUSES[] = {
    {0,   80, 3.5f},
    {2,  220, 3.5f},
    {4,  360, 3.5f},
    {1,  500, 3.5f},
    {5,  640, 3.5f},
    {3,  780, 3.5f},
    {0,  920, 3.5f},
    {2, 1060, 3.5f},
    {4, 1180, 3.5f},
};
static const int HOUSE_DECOR_COUNT = 9;

static void draw_house(const HouseDecor *h, const Assets *assets) {
    Texture2D tex = assets->houses[h->houseIdx];
    float dw = (float)tex.width  * h->scale;
    float dh = (float)tex.height * h->scale;
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst = {h->x - dw / 2.0f, (float)GROUND_Y - dh, dw, dh};
    DrawTexturePro(tex, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
}

static void draw_agent(const Agent *a, const Assets *assets) {
    // Source rect — negative width flips horizontally (east-facing)
    float fw = a->facingRight
               ? (float)SPRITE_FRAME_SIZE
               : -(float)SPRITE_FRAME_SIZE;
    float fx = a->facingRight
               ? (float)(a->animFrame * SPRITE_FRAME_SIZE)
               : (float)((a->animFrame + 1) * SPRITE_FRAME_SIZE);

    Rectangle src = {
        fx,
        (float)(SPRITE_WALK_ROW * SPRITE_FRAME_SIZE),
        fw,
        (float)SPRITE_FRAME_SIZE
    };

    Rectangle dst = {
        a->x - SPRITE_DISP / 2.0f,
        (float)GROUND_Y - SPRITE_DISP,
        SPRITE_DISP,
        SPRITE_DISP
    };

    Color tint = (a->tradeFlash > 0.0f) ? YELLOW : WHITE;
    DrawTexturePro(assets->sprites[a->spriteType], src, dst,
                   (Vector2){0, 0}, 0.0f, tint);

    // Status dot at agent feet
    Color dot;
    if (a->tradeFlash > 0.0f)        dot = YELLOW;
    else if (agent_is_buyer(a))       dot = (Color){60,  210,  90, 200};
    else if (agent_is_seller(a))      dot = (Color){220,  70,  70, 200};
    else                              dot = (Color){150, 150, 150, 180};
    DrawCircle((int)a->x, GROUND_Y + 3, 3, dot);
}

void render_world(const Agent *agents, int count, bool paused, int simSteps,
                  const Assets *assets) {
    DrawTexturePro(
        assets->background,
        (Rectangle){0, 0, (float)assets->background.width,
                          (float)assets->background.height},
        (Rectangle){0, 0, (float)SCREEN_W, (float)WORLD_AREA_H},
        (Vector2){0, 0}, 0.0f, WHITE
    );

    for (int i = 0; i < HOUSE_DECOR_COUNT; i++)
        draw_house(&HOUSES[i], assets);

    for (int i = 0; i < count; i++)
        draw_agent(&agents[i], assets);

    DrawRectangle(0, WORLD_AREA_H, SCREEN_W, 2, DARKGRAY);

    // Legend
    DrawRectangle(0, 0, 360, 28, (Color){0, 0, 0, 100});
    DrawCircle( 10, 14, 5, (Color){ 60, 210,  90, 255});
    DrawText("Buyer",    20, 7, 14, WHITE);
    DrawCircle( 78, 14, 5, (Color){220,  70,  70, 255});
    DrawText("Seller",   88, 7, 14, WHITE);
    DrawCircle(158, 14, 5, (Color){150, 150, 150, 255});
    DrawText("Neutral", 168, 7, 14, WHITE);
    DrawCircle(242, 14, 5, YELLOW);
    DrawText("Trading", 252, 7, 14, WHITE);

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

// Map basePersonalValue [20,80] to a blue→red color
static Color agent_color(float basePersonalValue, unsigned char alpha) {
    float t = (basePersonalValue - 20.0f) / 60.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (Color){
        (unsigned char)(55 + t * 200),
        30,
        (unsigned char)(255 - t * 200),
        alpha
    };
}

static void draw_axes_y(int px, int py, int pw, int ph,
                        float yMax, float refLine) {
    DrawLine(px, py, px, py + ph, LIGHTGRAY);
    DrawLine(px, py + ph, px + pw, py + ph, LIGHTGRAY);
    for (int step = 0; step <= 4; step++) {
        float v = yMax * (float)step / 4.0f;
        int y = py + ph - (int)((float)step / 4.0f * (float)ph);
        DrawLine(px - 4, y, px, y, LIGHTGRAY);
        char buf[12]; snprintf(buf, sizeof(buf), "%.0f", v);
        DrawText(buf, px - 28, y - 7, 11, LIGHTGRAY);
        DrawLine(px, y, px + pw, y, (Color){50,50,60,255});
    }
    if (refLine > 0.0f && refLine <= yMax) {
        int ry = py + ph - (int)(refLine / yMax * (float)ph);
        DrawLine(px, ry, px + pw, ry, (Color){255,200,0,110});
        DrawText("equil.", px + pw - 46, ry - 13, 11, (Color){255,200,0,180});
    }
}

// ---------------------------------------------------------------------------
// Left panel: money vs goods scatter (resource distribution)
// ---------------------------------------------------------------------------

static void draw_wealth_panel(const Agent *agents, int count,
                               int px, int py, int pw, int ph) {
    // Compute dynamic axis ranges
    float maxMoney = 1.0f;
    int   maxGoods = 1;
    for (int i = 0; i < count; i++) {
        if (agents[i].money > maxMoney) maxMoney = agents[i].money;
        if (agents[i].goods > maxGoods) maxGoods = agents[i].goods;
    }
    // Round up to nearest 50 / 5
    maxMoney = (float)(((int)(maxMoney / 50) + 1) * 50);
    if (maxGoods % 5 != 0) maxGoods = (maxGoods / 5 + 1) * 5;

    // Axes
    DrawLine(px, py,       px,       py + ph, LIGHTGRAY);
    DrawLine(px, py + ph,  px + pw,  py + ph, LIGHTGRAY);

    // Y-axis (money) gridlines
    for (int step = 0; step <= 4; step++) {
        float v = maxMoney * (float)step / 4.0f;
        int y = py + ph - (int)((float)step / 4.0f * (float)ph);
        DrawLine(px - 4, y, px, y, LIGHTGRAY);
        char buf[12]; snprintf(buf, sizeof(buf), "%.0f", v);
        DrawText(buf, px - 28, y - 7, 11, LIGHTGRAY);
        DrawLine(px, y, px + pw, y, (Color){50,50,60,255});
    }

    // X-axis (goods) gridlines
    for (int step = 0; step <= 4; step++) {
        int gv = maxGoods * step / 4;
        int x = px + (int)((float)step / 4.0f * (float)pw);
        DrawLine(x, py + ph, x, py + ph + 4, LIGHTGRAY);
        char buf[16]; snprintf(buf, sizeof(buf), "%d", gv);
        DrawText(buf, x - 5, py + ph + 6, 11, LIGHTGRAY);
        DrawLine(x, py, x, py + ph, (Color){50,50,60,255});
    }

    // Axis labels
    DrawText("$", px - 12, py - 2, 13, LIGHTGRAY);
    DrawText("goods →", px + pw - 52, py + ph + 6, 11, LIGHTGRAY);

    // Agent dots
    for (int i = 0; i < count; i++) {
        float gx = (float)agents[i].goods / (float)maxGoods;
        float gy = agents[i].money        / maxMoney;
        if (gx > 1.0f) gx = 1.0f;
        if (gy > 1.0f) gy = 1.0f;
        int sx = px + (int)(gx * (float)pw);
        int sy = py + ph - (int)(gy * (float)ph);

        Color col;
        if (agents[i].tradeFlash > 0.0f)     col = YELLOW;
        else if (agent_is_buyer(&agents[i]))  col = (Color){ 60, 210,  90, 200};
        else if (agent_is_seller(&agents[i])) col = (Color){220,  70,  70, 200};
        else                                  col = (Color){150, 150, 150, 180};
        DrawCircle(sx, sy, 3, col);
    }
}

// ---------------------------------------------------------------------------
// Right panel: per-agent expected value over time
// ---------------------------------------------------------------------------

static void draw_timeseries_panel(const AgentValueHistory *avh,
                                   const Agent *agents,
                                   int px, int py, int pw, int ph,
                                   float equilibrium) {
    draw_axes_y(px, py, pw, ph, 100.0f, equilibrium);

    if (avh->count < 2) return;

    // One thin line per agent, colored by basePersonalValue
    for (int ag = 0; ag < avh->agentCount; ag++) {
        Color col = agent_color(agents[ag].basePersonalValue, 55);
        float xScale = (float)pw / (float)(PRICE_HISTORY_SIZE - 1);
        for (int s = 1; s < avh->count; s++) {
            float v0 = avh_get(avh, ag, s - 1);
            float v1 = avh_get(avh, ag, s);
            float t0 = (float)(s - 1) * xScale;
            float t1 = (float)s       * xScale;
            int x0 = px + (int)t0, x1 = px + (int)t1;
            int y0 = py + ph - (int)(v0 / 100.0f * (float)ph);
            int y1 = py + ph - (int)(v1 / 100.0f * (float)ph);
            DrawLine(x0, y0, x1, y1, col);
        }
    }

    // Average line on top (bright white)
    float xScale = (float)pw / (float)(PRICE_HISTORY_SIZE - 1);
    for (int s = 1; s < avh->count; s++) {
        float v0 = avh_avg(avh, s - 1);
        float v1 = avh_avg(avh, s);
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

static const char *plot_title(PlotType t) {
    switch (t) {
        case PLOT_WEALTH:      return "Money vs Goods";
        case PLOT_EMV_HISTORY: return "Market Value History";
        default:               return "";
    }
}

// Draw the clickable header strip for one panel.
static void draw_panel_strip(int px, int py_strip, int pw, PlotType t) {
    Vector2 mouse = GetMousePosition();
    bool hover = mouse.x >= px && mouse.x <= px + pw &&
                 mouse.y >= py_strip && mouse.y <= py_strip + 16;

    Color bg  = hover ? (Color){50, 65, 95, 230} : (Color){28, 38, 58, 210};
    Color bdr = hover ? (Color){100,130,180,255}  : (Color){ 55, 72,100,200};
    DrawRectangle(px, py_strip, pw, 16, bg);
    DrawRectangleLines(px, py_strip, pw, 16, bdr);

    char label[64];
    snprintf(label, sizeof(label), "< %s >", plot_title(t));
    int tw = MeasureText(label, 12);
    DrawText(label, px + (pw - tw) / 2, py_strip + 2, 12,
             hover ? WHITE : (Color){180,190,210,255});
}

static void draw_panel(PlotType t, const AgentValueHistory *avh,
                       const Agent *agents, int agentCount,
                       int px, int py, int pw, int ph, float equilibrium) {
    switch (t) {
        case PLOT_WEALTH:
            draw_wealth_panel(agents, agentCount, px, py, pw, ph);
            break;
        case PLOT_EMV_HISTORY:
            draw_timeseries_panel(avh, agents, px, py, pw, ph, equilibrium);
            break;
        default: break;
    }
}

void render_plot(const AgentValueHistory *avh, const Agent *agents,
                 int agentCount, PlotType leftPlot, PlotType rightPlot) {
    DrawRectangle(0, WORLD_AREA_H + 2, SCREEN_W, SCREEN_H - WORLD_AREA_H - 2,
                  (Color){20,20,30,255});

    int half     = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int px_left  = PLOT_MARGIN_L;
    int px_right = PLOT_MARGIN_L + half + PANEL_GAP;
    int strip_y  = WORLD_AREA_H + 2;
    int py       = strip_y + 16 + PLOT_MARGIN_T;
    int pheight  = SCREEN_H - py - PLOT_MARGIN_B;

    float pvSum = 0.0f;
    for (int i = 0; i < agentCount; i++) pvSum += agents[i].basePersonalValue;
    float equilibrium = pvSum / (float)agentCount;

    draw_panel_strip(px_left,  strip_y, half, leftPlot);
    draw_panel_strip(px_right, strip_y, half, rightPlot);

    DrawLine(px_right - PANEL_GAP/2, WORLD_AREA_H + 4,
             px_right - PANEL_GAP/2, SCREEN_H - 4,
             (Color){60,60,70,255});

    draw_panel(leftPlot,  avh, agents, agentCount, px_left,  py, half, pheight, equilibrium);
    draw_panel(rightPlot, avh, agents, agentCount, px_right, py, half, pheight, equilibrium);
}

bool plot_cycle_click(PlotType *left, PlotType *right) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();

    int half     = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int px_left  = PLOT_MARGIN_L;
    int px_right = PLOT_MARGIN_L + half + PANEL_GAP;
    int strip_y  = WORLD_AREA_H + 2;

    if (m.y >= strip_y && m.y <= strip_y + 16) {
        if (m.x >= px_left && m.x <= px_left + half) {
            *left = (*left + 1) % PLOT_COUNT;
            return true;
        }
        if (m.x >= px_right && m.x <= px_right + half) {
            *right = (*right + 1) % PLOT_COUNT;
            return true;
        }
    }
    return false;
}
