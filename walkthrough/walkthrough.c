#include "walkthrough/walkthrough.h"
#include "walkthrough/scenes.h"
#include "econ/econ.h"
#include "econ/agent.h"
#include "econ/market.h"
#include "render/render.h"
#include "raygui.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Fixed plot bounds for the walkthrough
// ---------------------------------------------------------------------------

static void set_walkthrough_bounds(void) {
    // Value / price axes: 0–80
    g_bounds[PLOT_VALUATION_DISTRIBUTION][MARKET_WOOD].yMax = 80.0f;
    g_bounds[PLOT_PRICE_HISTORY         ][MARKET_WOOD].yMax = 80.0f;
    g_bounds[PLOT_SUPPLY_DEMAND         ][MARKET_WOOD].yMax = 80.0f;
    // Wealth scatter: goods 0–100 (xMax), money 0–2000 (yMax)
    g_bounds[PLOT_WEALTH][MARKET_WOOD].xMax = 100.0f;
    g_bounds[PLOT_WEALTH][MARKET_WOOD].yMax = 2000.0f;
    // Goods/money history panels
    g_bounds[PLOT_GOODS_HISTORY ][MARKET_WOOD].yMax = 100.0f;
    g_bounds[PLOT_MONEY_HISTORY ][MARKET_WOOD].yMax = 2000.0f;
    // Chair market bounds (scene 3) — values span 10–100
    g_bounds[PLOT_VALUATION_DISTRIBUTION][MARKET_CHAIR].yMax = 120.0f;
    g_bounds[PLOT_PRICE_HISTORY         ][MARKET_CHAIR].yMax = 120.0f;
    g_bounds[PLOT_SUPPLY_DEMAND         ][MARKET_CHAIR].yMax = 120.0f;
    g_bounds[PLOT_WEALTH][MARKET_CHAIR].xMax = 20.0f;
    g_bounds[PLOT_WEALTH][MARKET_CHAIR].yMax = 2000.0f;
    g_bounds[PLOT_GOODS_HISTORY][MARKET_CHAIR].yMax = 20.0f;
}

static void clear_walkthrough_bounds(void) {
    g_bounds[PLOT_VALUATION_DISTRIBUTION][MARKET_WOOD].yMax = 0.0f;
    g_bounds[PLOT_PRICE_HISTORY         ][MARKET_WOOD].yMax = 0.0f;
    g_bounds[PLOT_SUPPLY_DEMAND         ][MARKET_WOOD].yMax = 0.0f;
    g_bounds[PLOT_WEALTH][MARKET_WOOD].xMax = 0.0f;
    g_bounds[PLOT_WEALTH][MARKET_WOOD].yMax = 0.0f;
    g_bounds[PLOT_GOODS_HISTORY ][MARKET_WOOD].yMax = 0.0f;
    g_bounds[PLOT_MONEY_HISTORY ][MARKET_WOOD].yMax = 0.0f;
    g_bounds[PLOT_VALUATION_DISTRIBUTION][MARKET_CHAIR].yMax = 0.0f;
    g_bounds[PLOT_PRICE_HISTORY         ][MARKET_CHAIR].yMax = 0.0f;
    g_bounds[PLOT_SUPPLY_DEMAND         ][MARKET_CHAIR].yMax = 0.0f;
    g_bounds[PLOT_WEALTH][MARKET_CHAIR].xMax = 0.0f;
    g_bounds[PLOT_WEALTH][MARKET_CHAIR].yMax = 0.0f;
    g_bounds[PLOT_GOODS_HISTORY][MARKET_CHAIR].yMax = 0.0f;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static const StepDef *current_step(const WalkthroughState *wt) {
    return &SCENES[wt->scene].steps[wt->step];
}

const StepDef *walkthrough_current_step(const WalkthroughState *wt) {
    return current_step(wt);
}

static void apply_scene(WalkthroughState *wt, SimContext *ctx) {
    SCENES[wt->scene].init(ctx);
}

static void apply_step(const WalkthroughState *wt) {
    const StepDef *s = current_step(wt);
    if (s->init) s->init();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

static void open_popup(WalkthroughState *wt) {
    if (wt->seen[wt->scene][wt->step]) return;
    wt->seen[wt->scene][wt->step] = true;
    wt->popup_active = true;
    g_simulation.paused = true;
}

static void close_popup(WalkthroughState *wt) {
    wt->popup_active = false;
    g_simulation.paused = false;
    g_simulation.ticks_per_frame = 1;
}

void walkthrough_init(WalkthroughState *wt, SimContext *ctx) {
    wt->scene = 0; wt->step = 0; wt->active = true;
    wt->scene_changed = false;
    memset(wt->seen, 0, sizeof(wt->seen));
    set_walkthrough_bounds();
    apply_step(wt);
    apply_scene(wt, ctx);
    open_popup(wt);
}

bool walkthrough_next_step(WalkthroughState *wt, SimContext *ctx) {
    wt->scene_changed = false;
    if (wt->step + 1 < SCENES[wt->scene].stepCount) {
        wt->step++;
        apply_step(wt);
    } else if (wt->scene + 1 < SCENE_COUNT) {
        wt->scene++; wt->step = 0;
        apply_step(wt);
        apply_scene(wt, ctx);
        wt->scene_changed = true;
    } else {
        walkthrough_exit(wt);
        return false;
    }
    open_popup(wt);
    return true;
}

bool walkthrough_prev_step(WalkthroughState *wt, SimContext *ctx) {
    wt->scene_changed = false;
    if (wt->step > 0) {
        wt->step--;
        apply_step(wt);
    } else if (wt->scene > 0) {
        wt->scene--;
        wt->step = SCENES[wt->scene].stepCount - 1;
        apply_step(wt);
        apply_scene(wt, ctx);
        wt->scene_changed = true;
    } else {
        return false;
    }
    open_popup(wt);
    return true;
}

void walkthrough_restart(WalkthroughState *wt, SimContext *ctx) {
    apply_scene(wt, ctx);
    open_popup(wt);
}

void walkthrough_apply(WalkthroughState *wt, SimContext *ctx) {
    apply_step(wt);
    apply_scene(wt, ctx);
}

void walkthrough_exit(WalkthroughState *wt) {
    wt->active = false;
    clear_walkthrough_bounds();
    g_diminishing_returns = true;
    g_chop_wood_enabled    = true;
    g_leisure_enabled     = true;
    g_build_chairs_enabled = true;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.000f;
    g_chair_decay_rate    = 0.003f;
    g_inflation_enabled   = false;
}

// ---------------------------------------------------------------------------
// Input handler — keyboard shortcuts only; button clicks handled by GuiButton
// ---------------------------------------------------------------------------

bool walkthrough_handle_input(WalkthroughState *wt, SimContext *ctx) {
    if (!wt->active) return false;

    // While popup is showing, only keyboard can dismiss it
    // (mouse Ok button is handled by GuiButton in walkthrough_render_overlay)
    if (wt->popup_active) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            close_popup(wt);
        return false;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_N)) {
        walkthrough_next_step(wt, ctx); return true;
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_P)) {
        walkthrough_prev_step(wt, ctx); return true;
    }
    if (IsKeyPressed(KEY_R)) {
        walkthrough_restart(wt, ctx); return true;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        walkthrough_exit(wt); return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Word-wrap helper
// ---------------------------------------------------------------------------

static void draw_wrapped_text(const char *text, int x, int y, int maxW,
                               int fontSize, Color col) {
    char line[256];
    int lineStart = 0, lineLen = 0, lastSpace = -1;
    int ly = y;
    for (int i = 0; ; i++) {
        char c = text[i];
        if (c == '\0' || c == '\n') {
            int copyLen = lineLen < 255 ? lineLen : 255;
            strncpy(line, text + lineStart, (size_t)copyLen);
            line[copyLen] = '\0';
            DrawTextF(line, x, ly, fontSize, col);
            ly += fontSize + 4;
            if (c == '\0') break;
            lineStart = i + 1; lineLen = 0; lastSpace = -1;
        } else {
            if (c == ' ') lastSpace = i;
            lineLen++;
            int copyLen = lineLen < 255 ? lineLen : 255;
            strncpy(line, text + lineStart, (size_t)copyLen);
            line[copyLen] = '\0';
            if (MeasureTextF(line, fontSize) > maxW && lastSpace >= 0) {
                int cutLen = lastSpace - lineStart;
                if (cutLen > 255) cutLen = 255;
                strncpy(line, text + lineStart, (size_t)cutLen);
                line[cutLen] = '\0';
                DrawTextF(line, x, ly, fontSize, col);
                ly += fontSize + 4;
                lineStart = lastSpace + 1;
                lineLen = i - lineStart + 1;
                lastSpace = -1;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Overlay renderer — uses GuiButton for all interactive buttons
// ---------------------------------------------------------------------------

void walkthrough_render_overlay(WalkthroughState *wt, SimContext *ctx) {
    if (!wt->active) return;
    const StepDef  *s  = current_step(wt);
    const SceneDef *sc = &SCENES[wt->scene];

    // Nav bar background
    DrawRectangle(0, 0, GetScreenWidth(), WTHROUGH_NAV_H, (Color){18, 26, 40, 240});
    DrawLine(0, WTHROUGH_NAV_H, GetScreenWidth(), WTHROUGH_NAV_H, (Color){70, 90, 120, 255});

    // Navigation buttons
    if (GuiButton((Rectangle){10, 4, 70, WTHROUGH_NAV_H-8}, "< Prev"))
        walkthrough_prev_step(wt, ctx);
    if (GuiButton((Rectangle){GetScreenWidth()-80, 4, 70, WTHROUGH_NAV_H-8}, "Next >"))
        walkthrough_next_step(wt, ctx);
    if (GuiButton((Rectangle){GetScreenWidth()-170, 4, 70, WTHROUGH_NAV_H-8}, "Exit"))
        walkthrough_exit(wt);

    // Scene + step label (centred)
    char label[128];
    snprintf(label, sizeof(label), "%s  -  Step %d/%d",
             sc->title, wt->step + 1, sc->stepCount);
    int lw = MeasureTextF(label, 12);
    DrawTextF(label, (GetScreenWidth() - lw) / 2, 6, 12, (Color){200, 210, 230, 255});
    int stw = MeasureTextF(s->title, 11);
    DrawTextF(s->title, (GetScreenWidth() - stw) / 2, 21, 11, (Color){120, 160, 220, 200});

    // Step description text strip
    int textY = WTHROUGH_NAV_H + WORLD_VIEW_H - 75;
    DrawRectangle(0, textY, GetScreenWidth(), 75, (Color){0, 0, 0, 160});
    DrawLine(0, textY, GetScreenWidth(), textY, (Color){70, 90, 120, 200});
    draw_wrapped_text(s->text, 20, textY + 8, GetScreenWidth() - 40, 13,
                      (Color){200, 210, 230, 240});

    // Step intro popup
    if (wt->popup_active) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 160});

        int pw = 680, ph = 240;
        int px = (GetScreenWidth() - pw) / 2;
        int py = GetScreenHeight() / 2 - ph / 2;
        DrawRectangle(px, py, pw, ph, (Color){18, 26, 40, 250});
        DrawRectangleLines(px, py, pw, ph, (Color){80, 120, 180, 255});

        char title[128];
        snprintf(title, sizeof(title), "%s — %s", sc->title, s->title);
        int tw = MeasureTextF(title, 16);
        DrawTextF(title, (GetScreenWidth() - tw) / 2, py + 18, 16, (Color){180, 200, 240, 255});
        DrawLine(px + 20, py + 42, px + pw - 20, py + 42, (Color){60, 80, 120, 200});

        draw_wrapped_text(s->text, px + 24, py + 52, pw - 48, 14,
                          (Color){200, 210, 230, 240});

        int bw = 100, bh = 32;
        int bx = (GetScreenWidth() - bw) / 2;
        int by = py + ph - bh - 16;
        if (GuiButton((Rectangle){bx, by, bw, bh}, "Ok"))
            close_popup(wt);
    }
}
