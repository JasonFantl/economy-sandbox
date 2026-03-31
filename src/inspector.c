#include "inspector.h"
#include "render.h"
#include "assets.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Panel geometry
#define INS_X    840
#define INS_Y     30
#define INS_W    330
#define HDR_H     28
#define ROW_H     22
#define MKT_HDR_H 14
#define SEP_H      4
#define PAD        6

// Row Y positions
#define ROW_Y_FIRST      (INS_Y + HDR_H + PAD)
#define ROW_Y_ID         ROW_Y_FIRST
#define ROW_Y_ACTION     (ROW_Y_ID      + ROW_H)
#define ROW_Y_WOOD_HDR   (ROW_Y_ACTION  + ROW_H + SEP_H)
#define ROW_Y_WOOD_GOODS (ROW_Y_WOOD_HDR   + MKT_HDR_H)
#define ROW_Y_WOOD_S     (ROW_Y_WOOD_GOODS + ROW_H)
#define ROW_Y_WOOD_EMV   (ROW_Y_WOOD_S     + ROW_H)
#define ROW_Y_WOOD_PRICES (ROW_Y_WOOD_EMV  + ROW_H)
#define ROW_Y_CHAIR_HDR  (ROW_Y_WOOD_PRICES + ROW_H + SEP_H)
#define ROW_Y_CHAIR_GOODS (ROW_Y_CHAIR_HDR  + MKT_HDR_H)
#define ROW_Y_CHAIR_S    (ROW_Y_CHAIR_GOODS + ROW_H)
#define ROW_Y_CHAIR_EMV  (ROW_Y_CHAIR_S     + ROW_H)
#define ROW_Y_CHAIR_PRICES (ROW_Y_CHAIR_EMV + ROW_H)
#define ROW_Y_MONEY      (ROW_Y_CHAIR_PRICES + ROW_H + SEP_H)
#define ROW_Y_LEISURE    (ROW_Y_MONEY   + ROW_H)
#define ROW_Y_TARGET     (ROW_Y_LEISURE + ROW_H)

#define INS_H (HDR_H + 2*PAD + 2*ROW_H + SEP_H + MKT_HDR_H + 4*ROW_H + SEP_H + MKT_HDR_H + 4*ROW_H + SEP_H + 3*ROW_H)

static bool in_rect(float mx, float my, int rx, int ry, int rw, int rh) {
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
}

static void start_edit(Inspector *ins, EditField field, const Agent *a) {
    ins->editField = field;
    float val = 0.0f;
    switch (field) {
        case EDIT_WOOD_UTILITY:  val = a->econ.markets[MARKET_WOOD].maxUtility;      break;
        case EDIT_WOOD_PRICE:    val = a->econ.markets[MARKET_WOOD].priceExpectation; break;
        case EDIT_CHAIR_UTILITY: val = a->econ.markets[MARKET_CHAIR].maxUtility;     break;
        case EDIT_CHAIR_PRICE:   val = a->econ.markets[MARKET_CHAIR].priceExpectation; break;
        default: break;
    }
    snprintf(ins->inputBuf, sizeof(ins->inputBuf), "%.2f", val);
    ins->inputLen = (int)strlen(ins->inputBuf);
}

static void apply_edit(Inspector *ins, Agent *agents) {
    if (ins->editField == EDIT_NONE || ins->selectedId < 0) return;
    float val = (float)atof(ins->inputBuf);
    if (val < 0.1f) val = 0.1f;
    Agent *a = &agents[ins->selectedId];
    switch (ins->editField) {
        case EDIT_WOOD_UTILITY:  a->econ.markets[MARKET_WOOD].maxUtility       = val; break;
        case EDIT_WOOD_PRICE:    a->econ.markets[MARKET_WOOD].priceExpectation  = val; break;
        case EDIT_CHAIR_UTILITY: a->econ.markets[MARKET_CHAIR].maxUtility      = val; break;
        case EDIT_CHAIR_PRICE:   a->econ.markets[MARKET_CHAIR].priceExpectation = val; break;
        default: break;
    }
    ins->editField = EDIT_NONE;
}

void inspector_init(Inspector *ins) {
    ins->selectedId  = -1;
    ins->editField   = EDIT_NONE;
    ins->inputLen    = 0;
    ins->inputBuf[0] = '\0';
}

bool inspector_update(Inspector *ins, Agent *agents, int agentCount) {
    if (ins->editField != EDIT_NONE) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            bool validChar = (ch >= '0' && ch <= '9') || ch == '.' || ch == '-';
            if (validChar && ins->inputLen < 30) {
                ins->inputBuf[ins->inputLen++] = (char)ch;
                ins->inputBuf[ins->inputLen]   = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && ins->inputLen > 0)
            ins->inputBuf[--ins->inputLen] = '\0';
        if (IsKeyPressed(KEY_ENTER))
            apply_edit(ins, agents);
        if (IsKeyPressed(KEY_ESCAPE))
            ins->editField = EDIT_NONE;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();

    if (ins->selectedId >= 0 && in_rect(m.x, m.y, INS_X, INS_Y, INS_W, INS_H)) {
        // Close button
        if (in_rect(m.x, m.y, INS_X + INS_W - 24, INS_Y + 4, 20, 22)) {
            ins->selectedId = -1;
            ins->editField  = EDIT_NONE;
            return true;
        }
        // Editable rows
        if      (in_rect(m.x, m.y, INS_X, ROW_Y_WOOD_S,    INS_W, ROW_H))
            start_edit(ins, EDIT_WOOD_UTILITY,  &agents[ins->selectedId]);
        else if (in_rect(m.x, m.y, INS_X, ROW_Y_WOOD_EMV,  INS_W, ROW_H))
            start_edit(ins, EDIT_WOOD_PRICE,    &agents[ins->selectedId]);
        else if (in_rect(m.x, m.y, INS_X, ROW_Y_CHAIR_S,   INS_W, ROW_H))
            start_edit(ins, EDIT_CHAIR_UTILITY, &agents[ins->selectedId]);
        else if (in_rect(m.x, m.y, INS_X, ROW_Y_CHAIR_EMV, INS_W, ROW_H))
            start_edit(ins, EDIT_CHAIR_PRICE,   &agents[ins->selectedId]);
        else
            ins->editField = EDIT_NONE;
        return true;
    }

    // Agent sprite hit-test (only in world area)
    if (m.y <= WORLD_VIEW_H) {
        float hitR = (float)AGENT_DISP * 0.5f;
        for (int i = 0; i < agentCount; i++) {
            float dx = m.x - agents[i].body.x;
            float dy = m.y - agents[i].body.y;
            if (dx*dx + dy*dy <= hitR * hitR) {
                if (ins->selectedId == i) {
                    ins->selectedId = -1;
                    ins->editField  = EDIT_NONE;
                } else {
                    ins->selectedId = i;
                    ins->editField  = EDIT_NONE;
                }
                return true;
            }
        }
        ins->selectedId = -1;
        ins->editField  = EDIT_NONE;
    }

    return false;
}

// ---- rendering helpers -----------------------------------------------------

static void draw_row(int y, const char *label, const char *value,
                     bool editable, bool editing, Color valueColor) {
    Color bg = editing  ? (Color){ 20,  60,  90, 255}
             : editable ? (Color){ 35,  48,  60, 255}
                        : (Color){ 22,  30,  40, 255};
    DrawRectangle(INS_X, y, INS_W, ROW_H - 1, bg);
    if (editable && !editing)
        DrawRectangleLines(INS_X, y, INS_W, ROW_H - 1, (Color){70, 95, 115, 255});
    DrawText(label, INS_X + 8, y + 5, 12, (Color){170, 175, 185, 255});
    if (editing) {
        bool cursor = (int)(GetTime() * 2) % 2 == 0;
        char display[36];
        snprintf(display, sizeof(display), "%s%s", value, cursor ? "|" : " ");
        DrawText(display, INS_X + INS_W - MeasureText(display, 12) - 8, y + 5, 12, WHITE);
    } else {
        DrawText(value, INS_X + INS_W - MeasureText(value, 12) - 8, y + 5, 12, valueColor);
    }
}

// Draw a split row: left label+value and right label+value in the same row
static void draw_split_row(int y, const char *labelL, const char *valL, Color colL,
                                   const char *labelR, const char *valR, Color colR) {
    DrawRectangle(INS_X, y, INS_W, ROW_H - 1, (Color){22, 30, 40, 255});
    int mid = INS_X + INS_W / 2;
    DrawText(labelL, INS_X + 8,  y + 5, 12, (Color){170, 175, 185, 255});
    DrawText(valL,   mid - MeasureText(valL, 12) - 6, y + 5, 12, colL);
    DrawText(labelR, mid + 6,    y + 5, 12, (Color){170, 175, 185, 255});
    DrawText(valR,   INS_X + INS_W - MeasureText(valR, 12) - 8, y + 5, 12, colR);
}

static void draw_market_section_header(int y, const char *title, Color col) {
    DrawRectangle(INS_X, y, INS_W, MKT_HDR_H, (Color){28, 38, 55, 255});
    DrawRectangleLines(INS_X, y, INS_W, MKT_HDR_H, (Color){55, 72, 100, 200});
    DrawText(title, INS_X + 8, y + 2, 11, col);
}

void inspector_render(const Inspector *ins, const Agent *agents) {
    if (ins->selectedId < 0) return;
    const Agent *a = &agents[ins->selectedId];

    // Highlight ring around selected agent
    DrawCircleLines((int)a->body.x, (int)a->body.y, AGENT_DISP / 2 + 3, WHITE);

    // Panel shadow + background
    DrawRectangle(INS_X + 3, INS_Y + 3, INS_W, INS_H, (Color){0, 0, 0, 120});
    DrawRectangle(INS_X, INS_Y, INS_W, INS_H, (Color){15, 22, 32, 235});
    DrawRectangleLines(INS_X, INS_Y, INS_W, INS_H, (Color){90, 120, 150, 255});

    // Header
    DrawRectangle(INS_X, INS_Y, INS_W, HDR_H, (Color){28, 48, 78, 255});
    char title[32];
    snprintf(title, sizeof(title), "Agent #%d", a->body.id);
    DrawText(title, INS_X + 8, INS_Y + 7, 15, WHITE);
    DrawText("[X]", INS_X + INS_W - 30, INS_Y + 7, 14, (Color){200, 80, 80, 255});

    char buf[48];

    // ID + last action
    snprintf(buf, sizeof(buf), "%d", a->body.id);
    draw_row(ROW_Y_ID, "Agent ID", buf, false, false, LIGHTGRAY);

    const char *actName;
    Color actCol;
    switch (a->econ.lastAction) {
        case ACTION_CHOP:    actName = "Chopping";  actCol = (Color){160, 100,  40, 255}; break;
        case ACTION_BUILD:   actName = "Building";  actCol = (Color){220, 140,  60, 255}; break;
        default:             actName = "Leisure";   actCol = (Color){150, 150, 150, 255}; break;
    }
    draw_row(ROW_Y_ACTION, "Last Action", actName, false, false, actCol);

    // Separator + edit hint
    DrawRectangle(INS_X, ROW_Y_WOOD_HDR - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});
    DrawText("click to edit", INS_X + INS_W - 90, ROW_Y_WOOD_HDR - SEP_H + 1,
             10, (Color){90, 110, 130, 255});

    // ---- Wood market section ----
    draw_market_section_header(ROW_Y_WOOD_HDR, "WOOD MARKET", (Color){160, 100, 40, 255});

    const AgentMarket *mw = &a->econ.markets[MARKET_WOOD];
    snprintf(buf, sizeof(buf), "%d", mw->goods);
    draw_row(ROW_Y_WOOD_GOODS, "Goods", buf, false, false, (Color){200, 160, 80, 255});

    snprintf(buf, sizeof(buf), "%.2f", mw->maxUtility);
    draw_row(ROW_Y_WOOD_S, "Max Utility",
             ins->editField == EDIT_WOOD_UTILITY ? ins->inputBuf : buf,
             true, ins->editField == EDIT_WOOD_UTILITY,
             (Color){80, 140, 255, 255});

    snprintf(buf, sizeof(buf), "%.2f", mw->priceExpectation);
    draw_row(ROW_Y_WOOD_EMV, "Price Expect.",
             ins->editField == EDIT_WOOD_PRICE ? ins->inputBuf : buf,
             true, ins->editField == EDIT_WOOD_PRICE,
             (Color){60, 210, 90, 255});

    char buyBuf[24], sellBuf[24];
    snprintf(buyBuf,  sizeof(buyBuf),  "%.2f", marginal_buy_utility(mw));
    snprintf(sellBuf, sizeof(sellBuf), "%.2f", marginal_sell_utility(mw));
    draw_split_row(ROW_Y_WOOD_PRICES,
                   "Buy util:", buyBuf,  (Color){ 80, 200, 240, 255},
                   "Sell util:", sellBuf, (Color){240, 160,  80, 255});

    // ---- Chair market section ----
    DrawRectangle(INS_X, ROW_Y_CHAIR_HDR - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});
    draw_market_section_header(ROW_Y_CHAIR_HDR, "CHAIR MARKET", (Color){200, 140, 60, 255});

    const AgentMarket *mc = &a->econ.markets[MARKET_CHAIR];
    snprintf(buf, sizeof(buf), "%d", mc->goods);
    draw_row(ROW_Y_CHAIR_GOODS, "Goods", buf, false, false, (Color){200, 160, 80, 255});

    snprintf(buf, sizeof(buf), "%.2f", mc->maxUtility);
    draw_row(ROW_Y_CHAIR_S, "Max Utility",
             ins->editField == EDIT_CHAIR_UTILITY ? ins->inputBuf : buf,
             true, ins->editField == EDIT_CHAIR_UTILITY,
             (Color){80, 140, 255, 255});

    snprintf(buf, sizeof(buf), "%.2f", mc->priceExpectation);
    draw_row(ROW_Y_CHAIR_EMV, "Price Expect.",
             ins->editField == EDIT_CHAIR_PRICE ? ins->inputBuf : buf,
             true, ins->editField == EDIT_CHAIR_PRICE,
             (Color){60, 210, 90, 255});

    snprintf(buyBuf,  sizeof(buyBuf),  "%.2f", marginal_buy_utility(mc));
    snprintf(sellBuf, sizeof(sellBuf), "%.2f", marginal_sell_utility(mc));
    draw_split_row(ROW_Y_CHAIR_PRICES,
                   "Buy util:", buyBuf,  (Color){ 80, 200, 240, 255},
                   "Sell util:", sellBuf, (Color){240, 160,  80, 255});

    // ---- Shared stats ----
    DrawRectangle(INS_X, ROW_Y_MONEY - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});

    snprintf(buf, sizeof(buf), "%.2f", a->econ.money);
    draw_row(ROW_Y_MONEY, "Money", buf, false, false, (Color){255, 215, 0, 255});

    snprintf(buf, sizeof(buf), "%.2f", leisure_utility(&a->econ.leisure));
    draw_row(ROW_Y_LEISURE, "Leisure Utility", buf, false, false, (Color){150, 150, 150, 255});

    if (a->body.targetType == TARGET_AGENT)
        snprintf(buf, sizeof(buf), "Agent #%d", a->body.targetId);
    else
        snprintf(buf, sizeof(buf), "Pos %.0fpx", a->body.targetX);
    draw_row(ROW_Y_TARGET, "Target", buf, false, false, LIGHTGRAY);
}
