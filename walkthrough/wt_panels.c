#include "walkthrough/wt_panels.h"
#include "econ/econ.h"
#include "econ/market.h"
#include "econ/agent.h"
#include "raygui.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    p->woodUtilityDelta    = 5.0f;
    p->editWoodValue     = false;
    p->woodCountDelta    = 1;
    p->editWoodCount     = false;
    p->leisureValue      = 5.0f;
    p->editLeisure       = false;
    p->lastLeisure       = p->leisureValue;
    p->valueMarket       = MARKET_WOOD;
    p->goodsMarket       = MARKET_WOOD;
    snprintf(p->bufWoodValue, sizeof(p->bufWoodValue), "%.1f", p->woodUtilityDelta);
    snprintf(p->bufWoodCount, sizeof(p->bufWoodCount), "%d",   p->woodCountDelta);
    snprintf(p->bufLeisure,   sizeof(p->bufLeisure),   "%.1f", p->leisureValue);
    p->moneyDelta       = 100.0f;
    p->editMoney        = false;
    snprintf(p->bufMoney, sizeof(p->bufMoney), "%.1f", p->moneyDelta);
    p->beliefRateValue  = 0.1f;
    p->editBeliefRate   = false;
    p->lastBeliefRate   = p->beliefRateValue;
    snprintf(p->bufBeliefRate, sizeof(p->bufBeliefRate), "%.3f", p->beliefRateValue);
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
    if (flags & WT_INF_WOOD_VALUE)  rows++;
    if (flags & WT_INF_WOOD_COUNT)  rows++;
    if (flags & WT_INF_LEISURE)     rows++;
    if (flags & WT_INF_DIM_RETURNS) rows++;
    if (flags & WT_INF_INFLATION)   rows++;
    if (flags & WT_INF_MONEY)       rows++;
    if (flags & WT_INF_BELIEF_RATE) rows++;

    int contentH = rows * (WT_ROW_H + WT_SEP) + WT_SEP + WT_BTN_H + WT_PAD;
    int contentY = WT_Y + WT_HDR_H;
    DrawRectangle(px, contentY, WT_W, contentH, WT_BG);
    DrawRectangleLines(px, contentY, WT_W, contentH, WT_BORDER);

    int rowY = contentY + WT_SEP;

    // --- Value row (delta, with optional market selector) ---
    if (flags & WT_INF_WOOD_VALUE) {
        if (flags & WT_INF_MARKET_SEL) {
            GuiLabel((Rectangle){px + WT_MSLBL_DX, rowY, WT_MSLBL_W, WT_ROW_H - 2}, "Value:");
            bool was = p->editWoodValue;
            if (GuiTextBox((Rectangle){px + WT_MSBOX_DX, rowY, WT_MSBOX_W, WT_ROW_H - 2},
                           p->bufWoodValue, (int)sizeof(p->bufWoodValue), p->editWoodValue)) {
                if (was) {
                    if (IsKeyPressed(KEY_ENTER))
                        p->woodUtilityDelta = strtof(p->bufWoodValue, NULL);
                    else
                        snprintf(p->bufWoodValue, sizeof(p->bufWoodValue), "%.1f", p->woodUtilityDelta);
                }
                p->editWoodValue = !p->editWoodValue;
            }
            if (GuiButton((Rectangle){px + WT_MSMKT_DX, rowY, WT_MSMKT_W, WT_BTN_H},
                          p->valueMarket == MARKET_WOOD ? "Wood" : "Chair"))
                p->valueMarket = 1 - p->valueMarket;
            if (GuiButton((Rectangle){px + WT_MSAPL_DX, rowY, WT_MSAPL_W, WT_BTN_H}, "Add"))
                agents_adjust_valuations(agents, agentCount, agentCount,
                                         p->woodUtilityDelta, (MarketId)p->valueMarket);
        } else {
            GuiLabel((Rectangle){px + WT_DLBL_DX, rowY, WT_DLBL_W, WT_ROW_H - 2}, "Value:");
            bool was = p->editWoodValue;
            if (GuiTextBox((Rectangle){px + WT_DBOX_DX, rowY, WT_DBOX_W, WT_ROW_H - 2},
                           p->bufWoodValue, (int)sizeof(p->bufWoodValue), p->editWoodValue)) {
                if (was) {
                    if (IsKeyPressed(KEY_ENTER))
                        p->woodUtilityDelta = strtof(p->bufWoodValue, NULL);
                    else
                        snprintf(p->bufWoodValue, sizeof(p->bufWoodValue), "%.1f", p->woodUtilityDelta);
                }
                p->editWoodValue = !p->editWoodValue;
            }
            if (GuiButton((Rectangle){px + WT_DAPL_DX, rowY, WT_DAPL_W, WT_BTN_H}, "Add"))
                agents_adjust_valuations(agents, agentCount, agentCount,
                                         p->woodUtilityDelta, MARKET_WOOD);
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Goods row (delta, with optional market selector) ---
    if (flags & WT_INF_WOOD_COUNT) {
        if (flags & WT_INF_MARKET_SEL) {
            GuiLabel((Rectangle){px + WT_MSLBL_DX, rowY, WT_MSLBL_W, WT_ROW_H - 2}, "Goods:");
            bool was = p->editWoodCount;
            if (GuiTextBox((Rectangle){px + WT_MSBOX_DX, rowY, WT_MSBOX_W, WT_ROW_H - 2},
                           p->bufWoodCount, (int)sizeof(p->bufWoodCount), p->editWoodCount)) {
                if (was) {
                    if (IsKeyPressed(KEY_ENTER))
                        p->woodCountDelta = (int)strtol(p->bufWoodCount, NULL, 10);
                    else
                        snprintf(p->bufWoodCount, sizeof(p->bufWoodCount), "%d", p->woodCountDelta);
                }
                p->editWoodCount = !p->editWoodCount;
            }
            if (GuiButton((Rectangle){px + WT_MSMKT_DX, rowY, WT_MSMKT_W, WT_BTN_H},
                          p->goodsMarket == MARKET_WOOD ? "Wood" : "Chair"))
                p->goodsMarket = 1 - p->goodsMarket;
            if (GuiButton((Rectangle){px + WT_MSAPL_DX, rowY, WT_MSAPL_W, WT_BTN_H}, "Add"))
                agents_inject_goods(agents, agentCount, agentCount,
                                    p->woodCountDelta, (MarketId)p->goodsMarket);
        } else {
            GuiLabel((Rectangle){px + WT_DLBL_DX, rowY, WT_DLBL_W, WT_ROW_H - 2}, "Goods:");
            bool was = p->editWoodCount;
            if (GuiTextBox((Rectangle){px + WT_DBOX_DX, rowY, WT_DBOX_W, WT_ROW_H - 2},
                           p->bufWoodCount, (int)sizeof(p->bufWoodCount), p->editWoodCount)) {
                if (was) {
                    if (IsKeyPressed(KEY_ENTER))
                        p->woodCountDelta = (int)strtol(p->bufWoodCount, NULL, 10);
                    else
                        snprintf(p->bufWoodCount, sizeof(p->bufWoodCount), "%d", p->woodCountDelta);
                }
                p->editWoodCount = !p->editWoodCount;
            }
            if (GuiButton((Rectangle){px + WT_DAPL_DX, rowY, WT_DAPL_W, WT_BTN_H}, "Add"))
                agents_inject_goods(agents, agentCount, agentCount,
                                    p->woodCountDelta, MARKET_WOOD);
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Leisure row (setter): Enter applies, click-away reverts ---
    if (flags & WT_INF_LEISURE) {
        if (agentCount > 0) {
            float cur = agents[0].econ.leisureUtility;
            if (cur != p->lastLeisure && !p->editLeisure) {
                p->leisureValue = cur;
                p->lastLeisure  = cur;
                snprintf(p->bufLeisure, sizeof(p->bufLeisure), "%.1f", cur);
            }
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Leisure:");
        bool was = p->editLeisure;
        // Text box spans full remaining width (no Set button)
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_W - WT_PAD - WT_BOX_DX, WT_ROW_H - 2},
                       p->bufLeisure, (int)sizeof(p->bufLeisure), p->editLeisure)) {
            if (was) {
                if (IsKeyPressed(KEY_ENTER)) {
                    p->leisureValue = strtof(p->bufLeisure, NULL);
                    agents_set_leisure(agents, agentCount, p->leisureValue);
                    p->lastLeisure = p->leisureValue;
                } else {
                    snprintf(p->bufLeisure, sizeof(p->bufLeisure), "%.1f", p->leisureValue);
                }
            }
            p->editLeisure = !p->editLeisure;
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Diminishing returns toggle (immediate) ---
    if (flags & WT_INF_DIM_RETURNS) {
        GuiCheckBox((Rectangle){px + WT_LBL_DX, rowY + 4, WT_ROW_H - 8, WT_ROW_H - 8},
                    "Dim. Returns", &g_diminishing_returns);
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Inflation toggle (immediate) ---
    if (flags & WT_INF_INFLATION) {
        GuiCheckBox((Rectangle){px + WT_LBL_DX, rowY + 4, WT_ROW_H - 8, WT_ROW_H - 8},
                    "Inflation", &g_inflation_enabled);
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Money row (delta) ---
    if (flags & WT_INF_MONEY) {
        GuiLabel((Rectangle){px + WT_DLBL_DX, rowY, WT_DLBL_W, WT_ROW_H - 2}, "Money:");
        bool was = p->editMoney;
        if (GuiTextBox((Rectangle){px + WT_DBOX_DX, rowY, WT_DBOX_W, WT_ROW_H - 2},
                       p->bufMoney, (int)sizeof(p->bufMoney), p->editMoney)) {
            if (was) {
                if (IsKeyPressed(KEY_ENTER))
                    p->moneyDelta = strtof(p->bufMoney, NULL);
                else
                    snprintf(p->bufMoney, sizeof(p->bufMoney), "%.1f", p->moneyDelta);
            }
            p->editMoney = !p->editMoney;
        }
        if (GuiButton((Rectangle){px + WT_DAPL_DX, rowY, WT_DAPL_W, WT_BTN_H}, "Add"))
            agents_inject_money(agents, agentCount, agentCount, p->moneyDelta);
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Belief update rate row (setter) ---
    if (flags & WT_INF_BELIEF_RATE) {
        if (agentCount > 0) {
            float cur = agents[0].econ.beliefUpdateRate;
            if (cur != p->lastBeliefRate && !p->editBeliefRate) {
                p->beliefRateValue = cur;
                p->lastBeliefRate  = cur;
                snprintf(p->bufBeliefRate, sizeof(p->bufBeliefRate), "%.3f", cur);
            }
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Belief rate:");
        bool was = p->editBeliefRate;
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_W - WT_PAD - WT_BOX_DX, WT_ROW_H - 2},
                       p->bufBeliefRate, (int)sizeof(p->bufBeliefRate), p->editBeliefRate)) {
            if (was) {
                if (IsKeyPressed(KEY_ENTER)) {
                    p->beliefRateValue = strtof(p->bufBeliefRate, NULL);
                    agents_set_belief_rate(agents, agentCount, p->beliefRateValue);
                    p->lastBeliefRate = p->beliefRateValue;
                } else {
                    snprintf(p->bufBeliefRate, sizeof(p->bufBeliefRate), "%.3f", p->beliefRateValue);
                }
            }
            p->editBeliefRate = !p->editBeliefRate;
        }
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

void wt_wealth_config_init(WealthAxisConfig *c) {
    c->xAxis = WEALTH_AXIS_WOOD_COUNT;
    c->yAxis = WEALTH_AXIS_MONEY;
    c->axisMax[WEALTH_AXIS_MONEY]      = 2000.0f;
    c->axisMax[WEALTH_AXIS_WOOD_COUNT] = 100.0f;
    c->axisMax[WEALTH_AXIS_WOOD_UTIL]  = 80.0f;
    c->xEditMode = false;
    c->yEditMode = false;
    strncpy(c->dropdownText, "Money;Wood Count;Wood Utility", sizeof(c->dropdownText) - 1);
    c->dropdownText[sizeof(c->dropdownText) - 1] = '\0';
}

void wt_wealth_chair_config_init(WealthAxisConfig *c) {
    c->xAxis = WEALTH_AXIS_WOOD_COUNT;
    c->yAxis = WEALTH_AXIS_MONEY;
    c->axisMax[WEALTH_AXIS_MONEY]      = 2000.0f;
    c->axisMax[WEALTH_AXIS_WOOD_COUNT] = 20.0f;
    c->axisMax[WEALTH_AXIS_WOOD_UTIL]  = 240.0f;
    c->xEditMode = false;
    c->yEditMode = false;
    strncpy(c->dropdownText, "Money;Chair Count;Chair Utility", sizeof(c->dropdownText) - 1);
    c->dropdownText[sizeof(c->dropdownText) - 1] = '\0';
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
            // Market-selector style: Wood/Chair toggle; text box + market button (no Set)
            float curDecay = (p->decayMarket == MARKET_WOOD) ? g_wood_decay_rate : g_chair_decay_rate;
            if (!p->editDecay && curDecay != p->lastDecayRate) {
                p->decayRate     = curDecay;
                p->lastDecayRate = curDecay;
                snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", curDecay);
            }
            GuiLabel((Rectangle){px + WT_MSLBL_DX, rowY, WT_MSLBL_W, WT_ROW_H - 2}, "Decay:");
            // Text box takes space previously shared with Set button
            bool was = p->editDecay;
            if (GuiTextBox((Rectangle){px + WT_MSBOX_DX, rowY, WT_MSBOX_W + 4 + WT_MSAPL_W, WT_ROW_H - 2},
                           p->bufDecay, (int)sizeof(p->bufDecay), p->editDecay)) {
                if (was) {
                    if (IsKeyPressed(KEY_ENTER)) {
                        p->decayRate = strtof(p->bufDecay, NULL);
                        if (p->decayMarket == MARKET_WOOD) g_wood_decay_rate  = p->decayRate;
                        else                               g_chair_decay_rate = p->decayRate;
                        p->lastDecayRate = p->decayRate;
                    } else {
                        snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", p->decayRate);
                    }
                }
                p->editDecay = !p->editDecay;
            }
            if (GuiButton((Rectangle){px + WT_MSMKT_DX + WT_MSAPL_W + 4, rowY, WT_MSMKT_W, WT_BTN_H},
                          p->decayMarket == MARKET_WOOD ? "Wood" : "Chair")) {
                p->decayMarket = 1 - p->decayMarket;
                float newDecay = (p->decayMarket == MARKET_WOOD) ? g_wood_decay_rate : g_chair_decay_rate;
                p->decayRate     = newDecay;
                p->lastDecayRate = newDecay;
                snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", newDecay);
            }
        } else {
            // Setter style: wood only, no market selector
            p->decayMarket = MARKET_WOOD;
            if (!p->editDecay && g_wood_decay_rate != p->lastDecayRate) {
                p->decayRate     = g_wood_decay_rate;
                p->lastDecayRate = g_wood_decay_rate;
                snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", g_wood_decay_rate);
            }
            GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Decay:");
            bool was = p->editDecay;
            if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_W - WT_PAD - WT_BOX_DX, WT_ROW_H - 2},
                           p->bufDecay, (int)sizeof(p->bufDecay), p->editDecay)) {
                if (was) {
                    if (IsKeyPressed(KEY_ENTER)) {
                        p->decayRate      = strtof(p->bufDecay, NULL);
                        g_wood_decay_rate = p->decayRate;
                        p->lastDecayRate  = p->decayRate;
                    } else {
                        snprintf(p->bufDecay, sizeof(p->bufDecay), "%.4f", p->decayRate);
                    }
                }
                p->editDecay = !p->editDecay;
            }
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Chop yield row ---
    if (flags & WT_ENV_CHOP_YIELD) {
        if (!p->editChopYield && g_chop_yield != p->lastChopYield) {
            p->chopYield     = g_chop_yield;
            p->lastChopYield = g_chop_yield;
            snprintf(p->bufChopYield, sizeof(p->bufChopYield), "%d", p->chopYield);
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Yield:");
        bool was = p->editChopYield;
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_W - WT_PAD - WT_BOX_DX, WT_ROW_H - 2},
                       p->bufChopYield, (int)sizeof(p->bufChopYield), p->editChopYield)) {
            if (was) {
                if (IsKeyPressed(KEY_ENTER)) {
                    p->chopYield     = (int)strtol(p->bufChopYield, NULL, 10);
                    g_chop_yield     = p->chopYield;
                    p->lastChopYield = p->chopYield;
                } else {
                    snprintf(p->bufChopYield, sizeof(p->bufChopYield), "%d", p->chopYield);
                }
            }
            p->editChopYield = !p->editChopYield;
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Build cost row ---
    if (flags & WT_ENV_BUILD_COST) {
        if (!p->editWoodPerChair && g_wood_per_chair != p->lastWoodPerChair) {
            p->woodPerChair     = g_wood_per_chair;
            p->lastWoodPerChair = g_wood_per_chair;
            snprintf(p->bufWoodPerChair, sizeof(p->bufWoodPerChair), "%d", p->woodPerChair);
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2}, "Build:");
        bool was = p->editWoodPerChair;
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_W - WT_PAD - WT_BOX_DX, WT_ROW_H - 2},
                       p->bufWoodPerChair, (int)sizeof(p->bufWoodPerChair), p->editWoodPerChair)) {
            if (was) {
                if (IsKeyPressed(KEY_ENTER)) {
                    p->woodPerChair = (int)strtol(p->bufWoodPerChair, NULL, 10);
                    if (p->woodPerChair < 1) p->woodPerChair = 1;
                    g_wood_per_chair    = p->woodPerChair;
                    p->lastWoodPerChair = p->woodPerChair;
                } else {
                    snprintf(p->bufWoodPerChair, sizeof(p->bufWoodPerChair), "%d", p->woodPerChair);
                }
            }
            p->editWoodPerChair = !p->editWoodPerChair;
        }
        rowY += WT_ROW_H + WT_SEP;
    }

    (void)rowY;
}
