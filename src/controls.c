#include "controls.h"
#include "econ.h"
#include "market.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Panel geometry
#define PNL_X   10
#define PNL_Y   32
#define PNL_W  210
#define HDR_H   22
#define ROW_H   24
#define SEP      4
#define BTN_H   26
#define PAD      5
#define MKT_ROW_H 22
// Rows: N, money, [SEP], market selector, value, goods, [SEP], button
#define PNL_H  (HDR_H + SEP + ROW_H + ROW_H + SEP + MKT_ROW_H + ROW_H + ROW_H + SEP + BTN_H + PAD)

#define ROW_Y_N      (PNL_Y + HDR_H + SEP)
#define ROW_Y_MONEY  (ROW_Y_N + ROW_H)
#define ROW_Y_MKT    (ROW_Y_MONEY + ROW_H + SEP)
#define ROW_Y_F      (ROW_Y_MKT + MKT_ROW_H)
#define ROW_Y_GOODS  (ROW_Y_F + ROW_H)
#define BTN_Y        (ROW_Y_GOODS + ROW_H + SEP)

static bool in_rect(float mx, float my, int rx, int ry, int rw, int rh) {
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
}

static void start_edit(InfluencePanel *p, InfluenceEditField field) {
    p->editing = field;
    switch (field) {
        case INF_EDIT_NUM_AGENTS:  snprintf(p->buf, sizeof(p->buf), "%d",   p->numAgents);      break;
        case INF_EDIT_MONEY:       snprintf(p->buf, sizeof(p->buf), "%.1f", p->moneyDelta);     break;
        case INF_EDIT_VALUATION:   snprintf(p->buf, sizeof(p->buf), "%.1f", p->valuationDelta); break;
        case INF_EDIT_GOODS:       snprintf(p->buf, sizeof(p->buf), "%d",   p->goodsDelta);     break;
        default: break;
    }
    p->bufLen = (int)strlen(p->buf);
}

static void apply_edit(InfluencePanel *p) {
    switch (p->editing) {
        case INF_EDIT_NUM_AGENTS: { int v = atoi(p->buf); if (v > 0) p->numAgents = v; }  break;
        case INF_EDIT_MONEY:      p->moneyDelta     = (float)atof(p->buf);                 break;
        case INF_EDIT_VALUATION:  p->valuationDelta = (float)atof(p->buf);                 break;
        case INF_EDIT_GOODS:      p->goodsDelta     = atoi(p->buf);                        break;
        default: break;
    }
    p->editing = INF_EDIT_NONE;
}

void influence_panel_init(InfluencePanel *p) {
    p->numAgents      = 30;
    p->moneyDelta     = 100.0f;
    p->valuationDelta = 5.0f;
    p->goodsDelta     = 10;
    p->marketId       = MARKET_WOOD;
    p->editing        = INF_EDIT_NONE;
    p->bufLen         = 0;
    p->buf[0]         = '\0';
}

bool influence_panel_update(InfluencePanel *p, Agent *agents, int agentCount) {
    if (p->editing != INF_EDIT_NONE) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            bool ok = (ch >= '0' && ch <= '9') || ch == '.'
                    || (ch == '-' && p->bufLen == 0);
            if (ok && p->bufLen < 14) {
                p->buf[p->bufLen++] = (char)ch;
                p->buf[p->bufLen]   = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && p->bufLen > 0)
            p->buf[--p->bufLen] = '\0';
        if (IsKeyPressed(KEY_ENTER))  apply_edit(p);
        if (IsKeyPressed(KEY_ESCAPE)) p->editing = INF_EDIT_NONE;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();
    if (!in_rect(m.x, m.y, PNL_X, PNL_Y, PNL_W, PNL_H)) return false;

    if (in_rect(m.x, m.y, PNL_X, ROW_Y_N, PNL_W, ROW_H)) {
        if (p->editing == INF_EDIT_NUM_AGENTS) apply_edit(p);
        else start_edit(p, INF_EDIT_NUM_AGENTS);
        return true;
    }
    if (in_rect(m.x, m.y, PNL_X, ROW_Y_MONEY, PNL_W, ROW_H)) {
        if (p->editing == INF_EDIT_MONEY) apply_edit(p);
        else start_edit(p, INF_EDIT_MONEY);
        return true;
    }
    // Market toggle buttons
    int btnW = (PNL_W - 2*PAD - 4) / 2;
    int btnX_wood  = PNL_X + PAD;
    int btnX_chair = btnX_wood + btnW + 4;
    if (in_rect(m.x, m.y, btnX_wood,  ROW_Y_MKT, btnW, MKT_ROW_H)) {
        p->marketId = MARKET_WOOD;
        return true;
    }
    if (in_rect(m.x, m.y, btnX_chair, ROW_Y_MKT, btnW, MKT_ROW_H)) {
        p->marketId = MARKET_CHAIR;
        return true;
    }
    if (in_rect(m.x, m.y, PNL_X, ROW_Y_F, PNL_W, ROW_H)) {
        if (p->editing == INF_EDIT_VALUATION) apply_edit(p);
        else start_edit(p, INF_EDIT_VALUATION);
        return true;
    }
    if (in_rect(m.x, m.y, PNL_X, ROW_Y_GOODS, PNL_W, ROW_H)) {
        if (p->editing == INF_EDIT_GOODS) apply_edit(p);
        else start_edit(p, INF_EDIT_GOODS);
        return true;
    }
    // Apply button
    if (in_rect(m.x, m.y, PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H)) {
        if (p->editing != INF_EDIT_NONE) apply_edit(p);
        if (p->moneyDelta != 0.0f)     agents_inject_money(agents, agentCount, p->numAgents, p->moneyDelta);
        if (p->valuationDelta != 0.0f) agents_adjust_valuations(agents, agentCount, p->numAgents, p->valuationDelta, p->marketId);
        if (p->goodsDelta != 0)        agents_inject_goods(agents, agentCount, p->numAgents, p->goodsDelta, p->marketId);
        return true;
    }

    return true;
}

void influence_panel_render(const InfluencePanel *p) {
    DrawRectangle(PNL_X + 2, PNL_Y + 2, PNL_W, PNL_H, (Color){0, 0, 0, 100});
    DrawRectangle(PNL_X, PNL_Y, PNL_W, PNL_H, (Color){15, 22, 32, 220});
    DrawRectangleLines(PNL_X, PNL_Y, PNL_W, PNL_H, (Color){80, 110, 140, 255});

    DrawRectangle(PNL_X, PNL_Y, PNL_W, HDR_H, (Color){28, 48, 78, 255});
    DrawText("Influence Market", PNL_X + 6, PNL_Y + 5, 13, WHITE);

    // N row
    {
        bool editing = (p->editing == INF_EDIT_NUM_AGENTS);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(PNL_X, ROW_Y_N, PNL_W, ROW_H - 1, bg);
        DrawRectangleLines(PNL_X, ROW_Y_N, PNL_W, ROW_H - 1, (Color){70, 95, 115, 255});
        DrawText("N agents:", PNL_X + 6, ROW_Y_N + 5, 13, (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_N + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%d", p->numAgents);
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_N + 5, 13, (Color){80, 180, 255, 255});
        }
    }

    // Δ money row
    {
        bool editing = (p->editing == INF_EDIT_MONEY);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(PNL_X, ROW_Y_MONEY, PNL_W, ROW_H - 1, bg);
        DrawRectangleLines(PNL_X, ROW_Y_MONEY, PNL_W, ROW_H - 1, (Color){70, 95, 115, 255});
        DrawText("\xce\x94 money:", PNL_X + 6, ROW_Y_MONEY + 5, 13, (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_MONEY + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%+.0f", p->moneyDelta);
            Color col = (p->moneyDelta >= 0) ? (Color){60, 210, 90, 255} : (Color){220, 70, 70, 255};
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_MONEY + 5, 13, col);
        }
    }

    // Δ valuation row
    {
        bool editing = (p->editing == INF_EDIT_VALUATION);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(PNL_X, ROW_Y_F, PNL_W, ROW_H - 1, bg);
        DrawRectangleLines(PNL_X, ROW_Y_F, PNL_W, ROW_H - 1, (Color){70, 95, 115, 255});
        DrawText("\xce\x94 valuation:", PNL_X + 6, ROW_Y_F + 5, 13, (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_F + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%+.1f", p->valuationDelta);
            Color col = (p->valuationDelta >= 0) ? (Color){60, 210, 90, 255} : (Color){220, 70, 70, 255};
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_F + 5, 13, col);
        }
    }

    // Δ goods row
    {
        bool editing = (p->editing == INF_EDIT_GOODS);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(PNL_X, ROW_Y_GOODS, PNL_W, ROW_H - 1, bg);
        DrawRectangleLines(PNL_X, ROW_Y_GOODS, PNL_W, ROW_H - 1, (Color){70, 95, 115, 255});
        DrawText("\xce\x94 goods:", PNL_X + 6, ROW_Y_GOODS + 5, 13, (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_GOODS + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%+d", p->goodsDelta);
            Color col = (p->goodsDelta >= 0) ? (Color){60, 210, 90, 255} : (Color){220, 70, 70, 255};
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6, ROW_Y_GOODS + 5, 13, col);
        }
    }

    // Market selector row
    {
        int btnW = (PNL_W - 2*PAD - 4) / 2;
        int btnX_wood  = PNL_X + PAD;
        int btnX_chair = btnX_wood + btnW + 4;
        Vector2 mouse  = GetMousePosition();

        // Wood button
        bool hoverW  = in_rect(mouse.x, mouse.y, btnX_wood,  ROW_Y_MKT, btnW, MKT_ROW_H);
        bool activeW = (p->marketId == MARKET_WOOD);
        Color bgW  = activeW ? (Color){80, 55, 20, 255} : (hoverW ? (Color){50,35,15,255} : (Color){30,22,10,255});
        Color bdrW = activeW ? (Color){180,110,40,255}   : (Color){90,60,25,255};
        DrawRectangle(btnX_wood, ROW_Y_MKT, btnW, MKT_ROW_H, bgW);
        DrawRectangleLines(btnX_wood, ROW_Y_MKT, btnW, MKT_ROW_H, bdrW);
        const char *woodLbl = "Wood";
        DrawText(woodLbl,
                 btnX_wood + (btnW - MeasureText(woodLbl, 12)) / 2,
                 ROW_Y_MKT + 5, 12,
                 activeW ? (Color){220,140,60,255} : (Color){150,100,40,255});

        // Chair button
        bool hoverC  = in_rect(mouse.x, mouse.y, btnX_chair, ROW_Y_MKT, btnW, MKT_ROW_H);
        bool activeC = (p->marketId == MARKET_CHAIR);
        Color bgC  = activeC ? (Color){80, 55, 20, 255} : (hoverC ? (Color){50,35,15,255} : (Color){30,22,10,255});
        Color bdrC = activeC ? (Color){180,110,40,255}   : (Color){90,60,25,255};
        DrawRectangle(btnX_chair, ROW_Y_MKT, btnW, MKT_ROW_H, bgC);
        DrawRectangleLines(btnX_chair, ROW_Y_MKT, btnW, MKT_ROW_H, bdrC);
        const char *chairLbl = "Chair";
        DrawText(chairLbl,
                 btnX_chair + (btnW - MeasureText(chairLbl, 12)) / 2,
                 ROW_Y_MKT + 5, 12,
                 activeC ? (Color){220,140,60,255} : (Color){150,100,40,255});
    }

    // Apply button
    Vector2 mouse = GetMousePosition();
    bool hover = in_rect(mouse.x, mouse.y, PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H);
    Color btnBg  = hover ? (Color){60, 100, 160, 255} : (Color){40, 70, 120, 255};
    Color btnBdr = hover ? (Color){120, 170, 230, 255} : (Color){80, 120, 180, 255};
    DrawRectangle(PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H, btnBg);
    DrawRectangleLines(PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H, btnBdr);
    const char *btnLabel = "Apply Influence";
    DrawText(btnLabel,
             PNL_X + PAD + (PNL_W - 2*PAD - MeasureText(btnLabel, 13)) / 2,
             BTN_Y + 6, 13, WHITE);
}

// ---------------------------------------------------------------------------
// DecayRatePanel
// ---------------------------------------------------------------------------

// Positioned to the right of the influence panel
#define BR_X      (PNL_X + PNL_W + 8)
#define BR_W      PNL_W
#define BR_Y      PNL_Y
#define BR_HDR_H  22
#define BR_ROW_H  24
#define BR_SEP     4
#define BR_PAD     5
#define BR_H  (BR_HDR_H + BR_SEP + BR_ROW_H + BR_ROW_H + BR_ROW_H + BR_ROW_H + BR_PAD)

#define BR_ROW_Y_WOOD  (BR_Y + BR_HDR_H + BR_SEP)
#define BR_ROW_Y_CHAIR (BR_ROW_Y_WOOD  + BR_ROW_H)
#define BR_ROW_Y_CHOP  (BR_ROW_Y_CHAIR + BR_ROW_H)
#define BR_ROW_Y_DEBT  (BR_ROW_Y_CHOP  + BR_ROW_H)

void decay_rate_panel_init(DecayRatePanel *p) {
    p->editing = DECAY_EDIT_NONE;
    p->bufLen  = 0;
    p->buf[0]  = '\0';
}

bool decay_rate_panel_update(DecayRatePanel *p) {
    if (p->editing != DECAY_EDIT_NONE) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            bool ok = (ch >= '0' && ch <= '9')
                   || (ch == '.' && p->editing != DECAY_EDIT_CHOP_YIELD);
            if (ok && p->bufLen < 14) {
                p->buf[p->bufLen++] = (char)ch;
                p->buf[p->bufLen]   = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && p->bufLen > 0)
            p->buf[--p->bufLen] = '\0';
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            if (IsKeyPressed(KEY_ENTER)) {
                if (p->editing == DECAY_EDIT_CHOP_YIELD) {
                    int v = atoi(p->buf);
                    if (v >= 1) g_chop_yield_max = v;
                } else {
                    float val = (float)atof(p->buf);
                    if (val < 0.0f) val = 0.0f;
                    if (p->editing == DECAY_EDIT_WOOD)  g_wood_decay_rate  = val;
                    else                                g_chair_decay_rate = val;
                }
            }
            p->editing = DECAY_EDIT_NONE;
        }
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();
    if (!in_rect(m.x, m.y, BR_X, BR_Y, BR_W, BR_H)) return false;

    if (in_rect(m.x, m.y, BR_X, BR_ROW_Y_WOOD, BR_W, BR_ROW_H)) {
        if (p->editing == DECAY_EDIT_WOOD) {
            float val = (float)atof(p->buf);
            if (val >= 0.0f) g_wood_decay_rate = val;
            p->editing = DECAY_EDIT_NONE;
        } else {
            p->editing = DECAY_EDIT_WOOD;
            snprintf(p->buf, sizeof(p->buf), "%.4f", g_wood_decay_rate);
            p->bufLen = (int)strlen(p->buf);
        }
        return true;
    }
    if (in_rect(m.x, m.y, BR_X, BR_ROW_Y_CHAIR, BR_W, BR_ROW_H)) {
        if (p->editing == DECAY_EDIT_CHAIR) {
            float val = (float)atof(p->buf);
            if (val >= 0.0f) g_chair_decay_rate = val;
            p->editing = DECAY_EDIT_NONE;
        } else {
            p->editing = DECAY_EDIT_CHAIR;
            snprintf(p->buf, sizeof(p->buf), "%.4f", g_chair_decay_rate);
            p->bufLen = (int)strlen(p->buf);
        }
        return true;
    }
    if (in_rect(m.x, m.y, BR_X, BR_ROW_Y_CHOP, BR_W, BR_ROW_H)) {
        if (p->editing == DECAY_EDIT_CHOP_YIELD) {
            int v = atoi(p->buf);
            if (v >= 1) g_chop_yield_max = v;
            p->editing = DECAY_EDIT_NONE;
        } else {
            p->editing = DECAY_EDIT_CHOP_YIELD;
            snprintf(p->buf, sizeof(p->buf), "%d", g_chop_yield_max);
            p->bufLen = (int)strlen(p->buf);
        }
        return true;
    }
    if (in_rect(m.x, m.y, BR_X, BR_ROW_Y_DEBT, BR_W, BR_ROW_H)) {
        g_allow_debt = !g_allow_debt;
        return true;
    }
    return true;
}

void decay_rate_panel_render(const DecayRatePanel *p) {
    DrawRectangle(BR_X + 2, BR_Y + 2, BR_W, BR_H, (Color){0, 0, 0, 100});
    DrawRectangle(BR_X, BR_Y, BR_W, BR_H, (Color){15, 22, 32, 220});
    DrawRectangleLines(BR_X, BR_Y, BR_W, BR_H, (Color){80, 110, 140, 255});

    DrawRectangle(BR_X, BR_Y, BR_W, BR_HDR_H, (Color){28, 48, 78, 255});
    DrawText("Decay Rates (/unit/s)", BR_X + 6, BR_Y + 5, 13, WHITE);

    // Decay rate rows (wood, chair)
    for (int i = 0; i < 2; i++) {
        DecayEditField field = (i == 0) ? DECAY_EDIT_WOOD : DECAY_EDIT_CHAIR;
        int ry            = (i == 0) ? BR_ROW_Y_WOOD : BR_ROW_Y_CHAIR;
        const char *label = (i == 0) ? "Wood:" : "Chair:";
        float curVal      = (i == 0) ? g_wood_decay_rate : g_chair_decay_rate;
        bool editing      = (p->editing == field);

        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(BR_X, ry, BR_W, BR_ROW_H - 1, bg);
        DrawRectangleLines(BR_X, ry, BR_W, BR_ROW_H - 1, (Color){70, 95, 115, 255});
        DrawText(label, BR_X + 6, ry + 5, 13, (Color){170, 175, 185, 255});

        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, BR_X + BR_W - MeasureText(val, 13) - 6, ry + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%.4f", curVal);
            DrawText(val, BR_X + BR_W - MeasureText(val, 13) - 6, ry + 5, 13,
                     (Color){80, 180, 255, 255});
        }
    }

    // Chop yield row
    {
        bool editing = (p->editing == DECAY_EDIT_CHOP_YIELD);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(BR_X, BR_ROW_Y_CHOP, BR_W, BR_ROW_H - 1, bg);
        DrawRectangleLines(BR_X, BR_ROW_Y_CHOP, BR_W, BR_ROW_H - 1, (Color){70, 95, 115, 255});
        DrawText("Chop yield (max):", BR_X + 6, BR_ROW_Y_CHOP + 5, 13, (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, BR_X + BR_W - MeasureText(val, 13) - 6, BR_ROW_Y_CHOP + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%d", g_chop_yield_max);
            DrawText(val, BR_X + BR_W - MeasureText(val, 13) - 6, BR_ROW_Y_CHOP + 5, 13,
                     (Color){80, 180, 255, 255});
        }
    }

    // Allow-debt toggle row
    {
        Vector2 mouse = GetMousePosition();
        bool hover   = in_rect(mouse.x, mouse.y, BR_X, BR_ROW_Y_DEBT, BR_W, BR_ROW_H);
        bool active  = g_allow_debt;
        Color bg     = active ? (Color){60, 20, 20, 255} : (hover ? (Color){45, 45, 55, 255} : (Color){35, 48, 60, 255});
        Color bdr    = active ? (Color){200, 80, 80, 255} : (Color){70, 95, 115, 255};
        DrawRectangle(BR_X, BR_ROW_Y_DEBT, BR_W, BR_ROW_H - 1, bg);
        DrawRectangleLines(BR_X, BR_ROW_Y_DEBT, BR_W, BR_ROW_H - 1, bdr);
        DrawText("Allow Debt:", BR_X + 6, BR_ROW_Y_DEBT + 5, 13, (Color){170, 175, 185, 255});
        const char *status = active ? "ON" : "OFF";
        Color statusCol    = active ? (Color){220, 80, 80, 255} : (Color){80, 180, 255, 255};
        DrawText(status, BR_X + BR_W - MeasureText(status, 13) - 6,
                 BR_ROW_Y_DEBT + 5, 13, statusCol);
    }
}
