#include "walkthrough/wt_panels.h"
#include "econ/econ.h"
#include "econ/market.h"
#include "econ/agent.h"
#include "raygui.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Shared style
// ---------------------------------------------------------------------------

static const Color WT_BG     = {25, 35, 50, 240};
static const Color WT_BORDER = {80, 100, 130, 255};

// ---------------------------------------------------------------------------
// Geometry — panel width 240px.  Content width = 240 - 2*8 = 224.
//
// Setter row:      [Label 64][gap 4][TextBox 96][gap 4][Set   56]               = 224
// Delta row:       [Label 54][gap 4][TextBox 110][gap 4][Add   52]              = 224
// Mkt-select row:  [Label 52][gap 4][TextBox 68][gap 4][Mkt 52][gap 4][Btn 40] = 224
// ---------------------------------------------------------------------------

#define WT_W      240
#define WT_Y      32
#define WT_PAD    8
#define WT_ROW_H  24
#define WT_BTN_H  24
#define WT_SEP    4
#define WT_HDR_H  24

// Setter row geometry
#define WT_LBL_W   64
#define WT_BOX_W   96
#define WT_APL_W   56
#define WT_LBL_DX  (WT_PAD)
#define WT_BOX_DX  (WT_PAD + WT_LBL_W + 4)
#define WT_APL_DX  (WT_PAD + WT_LBL_W + 4 + WT_BOX_W + 4)

// Delta row geometry
#define WT_DLBL_W  54
#define WT_DBOX_W  110
#define WT_DAPL_W  52
#define WT_DLBL_DX (WT_PAD)
#define WT_DBOX_DX (WT_PAD + WT_DLBL_W + 4)
#define WT_DAPL_DX (WT_PAD + WT_DLBL_W + 4 + WT_DBOX_W + 4)

// Market-selector row: [Label 52][gap 4][TextBox 68][gap 4][Mkt 52][gap 4][Btn 40]
#define WT_MSLBL_W   52
#define WT_MSBOX_W   68
#define WT_MSMKT_W   52
#define WT_MSAPL_W   40
#define WT_MSLBL_DX  (WT_PAD)
#define WT_MSBOX_DX  (WT_PAD + WT_MSLBL_W + 4)
#define WT_MSMKT_DX  (WT_PAD + WT_MSLBL_W + 4 + WT_MSBOX_W + 4)
#define WT_MSAPL_DX  (WT_PAD + WT_MSLBL_W + 4 + WT_MSBOX_W + 4 + WT_MSMKT_W + 4)

// ---------------------------------------------------------------------------

void wt_influence_panel_init(WtInfluencePanel *p) {
    p->expanded          = false;
    p->restart_requested = false;
    p->woodValueDelta    = 5.0f;
    p->editWoodValue     = false;
    p->woodCountDelta    = 1;
    p->editWoodCount     = false;
    p->leisureValue      = 5.0f;
    p->editLeisure       = false;
    p->lastLeisure       = p->leisureValue;
    p->valueMarket       = MARKET_WOOD;
    p->goodsMarket       = MARKET_WOOD;
    snprintf(p->bufWoodValue, sizeof(p->bufWoodValue), "%.1f", p->woodValueDelta);
    snprintf(p->bufWoodCount, sizeof(p->bufWoodCount), "%d",   p->woodCountDelta);
    snprintf(p->bufLeisure,   sizeof(p->bufLeisure),   "%.1f", p->leisureValue);
    p->pendingInflation = g_inflation_enabled;
    p->lastInflation    = g_inflation_enabled;
}

void wt_influence_panel_render(WtInfluencePanel *p, Agent *agents, int agentCount,
                                int flags, int px) {
    // Collapsible header
    if (GuiButton((Rectangle){px, WT_Y, WT_W, WT_HDR_H},
                  p->expanded ? "v  Agents" : ">  Agents"))
        p->expanded = !p->expanded;

    if (!p->expanded) return;

    // Count visible rows to size the background (restart button always present)
    int rows = 0;
    if (flags & WT_INF_WOOD_VALUE) rows++;
    if (flags & WT_INF_WOOD_COUNT) rows++;
    if (flags & WT_INF_LEISURE)    rows++;
    if (flags & WT_INF_INFLATION)  rows++;

    int contentH = rows * (WT_ROW_H + WT_SEP) + WT_SEP + WT_BTN_H + WT_PAD;
    int contentY = WT_Y + WT_HDR_H;
    DrawRectangle(px, contentY, WT_W, contentH, WT_BG);
    DrawRectangleLines(px, contentY, WT_W, contentH, WT_BORDER);

    int rowY = contentY + WT_SEP;

    // --- Value row (delta, with optional market selector) ---
    if (flags & WT_INF_WOOD_VALUE) {
        if (flags & WT_INF_MARKET_SEL) {
            GuiLabel((Rectangle){px + WT_MSLBL_DX, rowY, WT_MSLBL_W, WT_ROW_H - 2}, "Value:");
            if (GuiTextBox((Rectangle){px + WT_MSBOX_DX, rowY, WT_MSBOX_W, WT_ROW_H - 2},
                           p->bufWoodValue, (int)sizeof(p->bufWoodValue), p->editWoodValue)) {
                p->editWoodValue = !p->editWoodValue;
                if (!p->editWoodValue)
                    p->woodValueDelta = strtof(p->bufWoodValue, NULL);
            }
            if (GuiButton((Rectangle){px + WT_MSMKT_DX, rowY, WT_MSMKT_W, WT_BTN_H},
                          p->valueMarket == MARKET_WOOD ? "Wood" : "Chair"))
                p->valueMarket = 1 - p->valueMarket;
            if (GuiButton((Rectangle){px + WT_MSAPL_DX, rowY, WT_MSAPL_W, WT_BTN_H}, "Add"))
                agents_adjust_valuations(agents, agentCount, agentCount,
                                         p->woodValueDelta, (MarketId)p->valueMarket);
        } else {
            GuiLabel((Rectangle){px + WT_DLBL_DX, rowY, WT_DLBL_W, WT_ROW_H - 2}, "Value:");
            if (GuiTextBox((Rectangle){px + WT_DBOX_DX, rowY, WT_DBOX_W, WT_ROW_H - 2},
                           p->bufWoodValue, (int)sizeof(p->bufWoodValue), p->editWoodValue)) {
                p->editWoodValue = !p->editWoodValue;
                if (!p->editWoodValue)
                    p->woodValueDelta = strtof(p->bufWoodValue, NULL);
            }
            if (GuiButton((Rectangle){px + WT_DAPL_DX, rowY, WT_DAPL_W, WT_BTN_H}, "Add"))
                agents_adjust_valuations(agents, agentCount, agentCount,
                                         p->woodValueDelta, MARKET_WOOD);
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Goods row (delta, with optional market selector) ---
    if (flags & WT_INF_WOOD_COUNT) {
        if (flags & WT_INF_MARKET_SEL) {
            GuiLabel((Rectangle){px + WT_MSLBL_DX, rowY, WT_MSLBL_W, WT_ROW_H - 2}, "Goods:");
            if (GuiTextBox((Rectangle){px + WT_MSBOX_DX, rowY, WT_MSBOX_W, WT_ROW_H - 2},
                           p->bufWoodCount, (int)sizeof(p->bufWoodCount), p->editWoodCount)) {
                p->editWoodCount = !p->editWoodCount;
                if (!p->editWoodCount)
                    p->woodCountDelta = (int)strtol(p->bufWoodCount, NULL, 10);
            }
            if (GuiButton((Rectangle){px + WT_MSMKT_DX, rowY, WT_MSMKT_W, WT_BTN_H},
                          p->goodsMarket == MARKET_WOOD ? "Wood" : "Chair"))
                p->goodsMarket = 1 - p->goodsMarket;
            if (GuiButton((Rectangle){px + WT_MSAPL_DX, rowY, WT_MSAPL_W, WT_BTN_H}, "Add"))
                agents_inject_goods(agents, agentCount, agentCount,
                                    p->woodCountDelta, (MarketId)p->goodsMarket);
        } else {
            GuiLabel((Rectangle){px + WT_DLBL_DX, rowY, WT_DLBL_W, WT_ROW_H - 2}, "Goods:");
            if (GuiTextBox((Rectangle){px + WT_DBOX_DX, rowY, WT_DBOX_W, WT_ROW_H - 2},
                           p->bufWoodCount, (int)sizeof(p->bufWoodCount), p->editWoodCount)) {
                p->editWoodCount = !p->editWoodCount;
                if (!p->editWoodCount)
                    p->woodCountDelta = (int)strtol(p->bufWoodCount, NULL, 10);
            }
            if (GuiButton((Rectangle){px + WT_DAPL_DX, rowY, WT_DAPL_W, WT_BTN_H}, "Add"))
                agents_inject_goods(agents, agentCount, agentCount,
                                    p->woodCountDelta, MARKET_WOOD);
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Leisure row (setter) ---
    if (flags & WT_INF_LEISURE) {
        // Sync only when the global changed externally since last frame
        if (agentCount > 0) {
            float cur = agents[0].econ.leisureUtility;
            if (cur != p->lastLeisure) {
                p->leisureValue = cur;
                p->lastLeisure  = cur;
                snprintf(p->bufLeisure, sizeof(p->bufLeisure), "%.1f", cur);
            }
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Leisure:");
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_BOX_W, WT_ROW_H - 2},
                       p->bufLeisure, (int)sizeof(p->bufLeisure), p->editLeisure)) {
            p->editLeisure = !p->editLeisure;
            if (!p->editLeisure)
                p->leisureValue = strtof(p->bufLeisure, NULL);
        }
        bool leisureMatch = (agentCount > 0 && p->leisureValue == agents[0].econ.leisureUtility);
        if (leisureMatch) GuiSetState(STATE_DISABLED);
        if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set"))
            agents_set_leisure(agents, agentCount, p->leisureValue);
        if (leisureMatch) GuiSetState(STATE_NORMAL);
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Inflation toggle (checkbox + Set) ---
    if (flags & WT_INF_INFLATION) {
        if (p->lastInflation != g_inflation_enabled) {
            p->pendingInflation = g_inflation_enabled;
            p->lastInflation    = g_inflation_enabled;
        }
        GuiCheckBox((Rectangle){px + WT_LBL_DX, rowY + 4, WT_ROW_H - 8, WT_ROW_H - 8},
                    "Inflation", &p->pendingInflation);
        bool inflMatch = (p->pendingInflation == g_inflation_enabled);
        if (inflMatch) GuiSetState(STATE_DISABLED);
        if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set"))
            g_inflation_enabled = p->pendingInflation;
        if (inflMatch) GuiSetState(STATE_NORMAL);
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Restart button (always at bottom) ---
    rowY += WT_SEP;
    if (GuiButton((Rectangle){px + WT_PAD, rowY, WT_W - 2*WT_PAD, WT_BTN_H}, "Restart"))
        p->restart_requested = true;
}

// ---------------------------------------------------------------------------
// Environment Panel
// ---------------------------------------------------------------------------

void wt_environment_panel_init(WtEnvironmentPanel *p) {
    p->expanded          = false;
    p->decayMarket       = MARKET_WOOD;
    p->decayRate         = g_wood_decay_rate;
    p->editDecay         = false;
    p->lastDecayRate     = p->decayRate;
    p->chopYield         = g_chop_yield;
    p->editChopYield     = false;
    p->lastChopYield     = p->chopYield;
    p->woodPerChair      = g_wood_per_chair;
    p->editWoodPerChair  = false;
    p->lastWoodPerChair  = p->woodPerChair;
    snprintf(p->bufDecay,        sizeof(p->bufDecay),        "%.4f", p->decayRate);
    snprintf(p->bufChopYield,    sizeof(p->bufChopYield),    "%d",   p->chopYield);
    snprintf(p->bufWoodPerChair, sizeof(p->bufWoodPerChair), "%d",   p->woodPerChair);
}

void wt_environment_panel_render(WtEnvironmentPanel *p, int flags, int px) {
    if (GuiButton((Rectangle){px, WT_Y, WT_W, WT_HDR_H},
                  p->expanded ? "v  Environment" : ">  Environment"))
        p->expanded = !p->expanded;

    if (!p->expanded) return;

    int rows = 0;
    if (flags & WT_ENV_WOOD_DECAY) rows++;
    if (flags & WT_ENV_CHOP_YIELD) rows++;
    if (flags & WT_ENV_BUILD_COST) rows++;

    int contentH = rows * (WT_ROW_H + WT_SEP) + WT_PAD;
    int contentY = WT_Y + WT_HDR_H;
    DrawRectangle(px, contentY, WT_W, contentH, WT_BG);
    DrawRectangleLines(px, contentY, WT_W, contentH, WT_BORDER);

    int rowY = contentY + WT_SEP;

    // --- Decay rate row ---
    if (flags & WT_ENV_WOOD_DECAY) {
        if (flags & WT_ENV_DECAY_MARKET_SEL) {
            // Market-selector style: Wood/Chair toggle button
            float curDecay = (p->decayMarket == MARKET_WOOD) ? g_wood_decay_rate : g_chair_decay_rate;
            if (curDecay != p->lastDecayRate) {
                p->decayRate     = curDecay;
                p->lastDecayRate = curDecay;
                snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", curDecay);
            }
            GuiLabel((Rectangle){px + WT_MSLBL_DX, rowY, WT_MSLBL_W, WT_ROW_H - 2}, "Decay:");
            if (GuiTextBox((Rectangle){px + WT_MSBOX_DX, rowY, WT_MSBOX_W, WT_ROW_H - 2},
                           p->bufDecay, (int)sizeof(p->bufDecay), p->editDecay)) {
                p->editDecay = !p->editDecay;
                if (!p->editDecay)
                    p->decayRate = strtof(p->bufDecay, NULL);
            }
            if (GuiButton((Rectangle){px + WT_MSMKT_DX, rowY, WT_MSMKT_W, WT_BTN_H},
                          p->decayMarket == MARKET_WOOD ? "Wood" : "Chair")) {
                p->decayMarket = 1 - p->decayMarket;
                float newDecay = (p->decayMarket == MARKET_WOOD) ? g_wood_decay_rate : g_chair_decay_rate;
                p->decayRate     = newDecay;
                p->lastDecayRate = newDecay;
                snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", newDecay);
            }
            if (GuiButton((Rectangle){px + WT_MSAPL_DX, rowY, WT_MSAPL_W, WT_BTN_H}, "Set")) {
                if (p->decayMarket == MARKET_WOOD) g_wood_decay_rate  = p->decayRate;
                else                               g_chair_decay_rate = p->decayRate;
                p->lastDecayRate = p->decayRate;
            }
        } else {
            // Setter style: wood only, no market selector; reset decayMarket for clean scene-3 entry
            p->decayMarket = MARKET_WOOD;
            if (g_wood_decay_rate != p->lastDecayRate) {
                p->decayRate     = g_wood_decay_rate;
                p->lastDecayRate = g_wood_decay_rate;
                snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", g_wood_decay_rate);
            }
            GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Decay:");
            if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_BOX_W, WT_ROW_H - 2},
                           p->bufDecay, (int)sizeof(p->bufDecay), p->editDecay)) {
                p->editDecay = !p->editDecay;
                if (!p->editDecay)
                    p->decayRate = strtof(p->bufDecay, NULL);
            }
            if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set")) {
                g_wood_decay_rate = p->decayRate;
                p->lastDecayRate  = p->decayRate;
            }
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Chop yield row ---
    if (flags & WT_ENV_CHOP_YIELD) {
        // Sync only when the global changed externally since last frame
        if (g_chop_yield != p->lastChopYield) {
            p->chopYield     = g_chop_yield;
            p->lastChopYield = g_chop_yield;
            snprintf(p->bufChopYield, sizeof(p->bufChopYield), "%d", p->chopYield);
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2},
                 "Yield:");
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_BOX_W, WT_ROW_H - 2},
                       p->bufChopYield, (int)sizeof(p->bufChopYield), p->editChopYield)) {
            p->editChopYield = !p->editChopYield;
            if (!p->editChopYield)
                p->chopYield = (int)strtol(p->bufChopYield, NULL, 10);
        }
        if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set"))
            g_chop_yield = p->chopYield;
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Build cost row ---
    if (flags & WT_ENV_BUILD_COST) {
        // Sync with current global
        if (g_wood_per_chair != p->lastWoodPerChair) {
            p->woodPerChair     = g_wood_per_chair;
            p->lastWoodPerChair = g_wood_per_chair;
            snprintf(p->bufWoodPerChair, sizeof(p->bufWoodPerChair), "%d", p->woodPerChair);
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2},
                 "Build:");
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_BOX_W, WT_ROW_H - 2},
                       p->bufWoodPerChair, (int)sizeof(p->bufWoodPerChair), p->editWoodPerChair)) {
            p->editWoodPerChair = !p->editWoodPerChair;
            if (!p->editWoodPerChair)
                p->woodPerChair = (int)strtol(p->bufWoodPerChair, NULL, 10);
        }
        if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set")) {
            if (p->woodPerChair < 1) p->woodPerChair = 1;
            g_wood_per_chair    = p->woodPerChair;
            p->lastWoodPerChair = p->woodPerChair;
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    (void)rowY;
}
