#include "render/controls.h"
#include "econ/econ.h"
#include "econ/market.h"
#include <stdbool.h>
#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Influence Panel geometry
// ---------------------------------------------------------------------------
// Label text is drawn LEFT of the box bounds by raygui (outside bounds.x).
// BOX_X therefore leaves room on the left for the widest label.

#define PNL_X     10
#define PNL_Y     32
#define PNL_W    210
#define ROW_H     24
#define MKT_H     22
#define BTN_H     26
#define SEP        4
#define PAD        8
#define LBL_W     90    // space reserved to the left of each value box

#define HDR_H   24

#define BOX_X   (PNL_X + PAD + LBL_W + 2)
#define BOX_W   (PNL_W - PAD - LBL_W - 2 - PAD/2)

#define ROW1_Y  (PNL_Y + HDR_H + 2)
#define ROW2_Y  (ROW1_Y + ROW_H)
#define MKT_Y   (ROW2_Y + ROW_H + SEP)
#define ROW3_Y  (MKT_Y  + MKT_H)
#define ROW4_Y  (ROW3_Y + ROW_H)
#define ROW5_Y  (ROW4_Y + ROW_H)
#define BTN_Y   (ROW5_Y + ROW_H + SEP)
#define PNL_H   (BTN_Y - PNL_Y + BTN_H + PAD / 2)

void influence_panel_init(InfluencePanel *p) {
    p->expanded       = false;
    p->numAgents      = 30;
    p->moneyDelta     = 100.0f;
    p->valuationDelta = 5.0f;
    p->goodsDelta     = 10;
    p->marketId       = MARKET_WOOD;
    p->leisureDelta   = 5.0f;
    p->editN = p->editMoney = p->editValuation = p->editGoods = p->editMarket = p->editLeisure = false;
    snprintf(p->bufMoney,     sizeof(p->bufMoney),     "%.1f", p->moneyDelta);
    snprintf(p->bufValuation, sizeof(p->bufValuation), "%.1f", p->valuationDelta);
    snprintf(p->bufLeisure,   sizeof(p->bufLeisure),   "%.1f", p->leisureDelta);
}

static const Color PANEL_BG   = {25, 35, 50, 240};
static const Color PANEL_BORDER = {80, 100, 130, 255};

void influence_panel_render(InfluencePanel *p, Agent *agents, int agentCount) {
    // Collapsible header button
    if (GuiButton((Rectangle){PNL_X, PNL_Y, PNL_W, HDR_H},
                  p->expanded ? "v  Influence Market" : ">  Influence Market"))
        p->expanded = !p->expanded;

    if (!p->expanded) return;

    // Opaque background for content area
    int contentH = PNL_H - HDR_H;
    DrawRectangle(PNL_X, PNL_Y + HDR_H, PNL_W, contentH, PANEL_BG);
    DrawRectangleLines(PNL_X, PNL_Y + HDR_H, PNL_W, contentH, PANEL_BORDER);

    // N agents
    if (GuiValueBox((Rectangle){BOX_X, ROW1_Y, BOX_W, ROW_H-2},
                    "N agents:", &p->numAgents, 1, agentCount, p->editN))
        p->editN = !p->editN;

    // Delta money (GuiTextBox allows '-' for negative values)
    GuiLabel((Rectangle){PNL_X + PAD, ROW2_Y, LBL_W, ROW_H-2}, "Money:");
    if (GuiTextBox((Rectangle){BOX_X, ROW2_Y, BOX_W, ROW_H-2},
                   p->bufMoney, (int)sizeof(p->bufMoney), p->editMoney)) {
        p->editMoney = !p->editMoney;
        if (!p->editMoney) p->moneyDelta = strtof(p->bufMoney, NULL);
    }

    // Delta valuation (GuiTextBox allows '-' for negative values)
    GuiLabel((Rectangle){PNL_X + PAD, ROW3_Y, LBL_W, ROW_H-2}, "Valuation:");
    if (GuiTextBox((Rectangle){BOX_X, ROW3_Y, BOX_W, ROW_H-2},
                   p->bufValuation, (int)sizeof(p->bufValuation), p->editValuation)) {
        p->editValuation = !p->editValuation;
        if (!p->editValuation) p->valuationDelta = strtof(p->bufValuation, NULL);
    }

    // Delta goods
    if (GuiValueBox((Rectangle){BOX_X, ROW4_Y, BOX_W, ROW_H-2},
                    "Goods:", &p->goodsDelta, -100, 100, p->editGoods))
        p->editGoods = !p->editGoods;

    // Delta leisure
    GuiLabel((Rectangle){PNL_X + PAD, ROW5_Y, LBL_W, ROW_H-2}, "Leisure:");
    if (GuiTextBox((Rectangle){BOX_X, ROW5_Y, BOX_W, ROW_H-2},
                   p->bufLeisure, (int)sizeof(p->bufLeisure), p->editLeisure)) {
        p->editLeisure = !p->editLeisure;
        if (!p->editLeisure) p->leisureDelta = strtof(p->bufLeisure, NULL);
    }

    // Apply button
    if (GuiButton((Rectangle){PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H},
                  "Apply Influence")) {
        p->editMarket = false;  // close dropdown on apply
        if (p->moneyDelta != 0.0f)
            agents_inject_money(agents, agentCount, p->numAgents, p->moneyDelta);
        if (p->valuationDelta != 0.0f)
            agents_adjust_valuations(agents, agentCount, p->numAgents,
                                     p->valuationDelta, p->marketId);
        if (p->goodsDelta != 0)
            agents_inject_goods(agents, agentCount, p->numAgents,
                                p->goodsDelta, p->marketId);
        if (p->leisureDelta != 0.0f)
            agents_adjust_leisure(agents, agentCount, p->numAgents, p->leisureDelta);
    }

    // Market dropdown drawn last so it overlays other widgets when open
    int mkt = (int)p->marketId;
    if (GuiDropdownBox((Rectangle){PNL_X + PAD, MKT_Y, PNL_W - 2*PAD, MKT_H},
                       "Wood;Chair", &mkt, p->editMarket))
        p->editMarket = !p->editMarket;
    p->marketId = (MarketId)mkt;
}

// ---------------------------------------------------------------------------
// Decay Rate Panel geometry
// ---------------------------------------------------------------------------

#define BR_X    (PNL_X + PNL_W + 8)
#define BR_Y    PNL_Y
#define BR_W    PNL_W
#define BR_LW   100
#define BR_BOX_X  (BR_X + PAD + BR_LW + 2)
#define BR_BOX_W  (BR_W - PAD - BR_LW - 2 - PAD/2)

#define BR_ROW1_Y  (BR_Y + HDR_H + 2)
#define BR_ROW2_Y  (BR_ROW1_Y + ROW_H)
#define BR_ROW3_Y  (BR_ROW2_Y + ROW_H)
#define BR_ROW4_Y  (BR_ROW3_Y + ROW_H)
#define BR_ROW5_Y  (BR_ROW4_Y + ROW_H)
#define BR_BTN_Y   (BR_ROW5_Y + ROW_H + SEP)
#define BR_H       (BR_BTN_Y - BR_Y + BTN_H + PAD/2)

void decay_rate_panel_init(DecayRatePanel *p) {
    p->expanded = false;
    p->editWood = p->editChair = p->editChop = false;
    p->pendingWoodDecay  = g_wood_decay_rate;
    p->pendingChairDecay = g_chair_decay_rate;
    p->pendingChopYield  = g_chop_yield;
    p->pendingDisableExecuting = g_disable_executing_trade;
    p->pendingInflation  = g_inflation_enabled;
    snprintf(p->bufWood,  sizeof(p->bufWood),  "%.4f", p->pendingWoodDecay);
    snprintf(p->bufChair, sizeof(p->bufChair), "%.4f", p->pendingChairDecay);
}

void decay_rate_panel_render(DecayRatePanel *p) {
    // Collapsible header button
    if (GuiButton((Rectangle){BR_X, BR_Y, BR_W, HDR_H},
                  p->expanded ? "v  Sim Controls" : ">  Sim Controls"))
        p->expanded = !p->expanded;

    if (!p->expanded) return;

    // Opaque background for content area
    int contentH = BR_H - HDR_H;
    DrawRectangle(BR_X, BR_Y + HDR_H, BR_W, contentH, PANEL_BG);
    DrawRectangleLines(BR_X, BR_Y + HDR_H, BR_W, contentH, PANEL_BORDER);

    // Wood decay rate (staged)
    if (GuiValueBoxFloat((Rectangle){BR_BOX_X, BR_ROW1_Y, BR_BOX_W, ROW_H-2},
                         "Wood:", p->bufWood, &p->pendingWoodDecay, p->editWood))
        p->editWood = !p->editWood;

    // Chair decay rate (staged)
    if (GuiValueBoxFloat((Rectangle){BR_BOX_X, BR_ROW2_Y, BR_BOX_W, ROW_H-2},
                         "Chair:", p->bufChair, &p->pendingChairDecay, p->editChair))
        p->editChair = !p->editChair;

    // Chop yield (staged)
    if (GuiValueBox((Rectangle){BR_BOX_X, BR_ROW3_Y, BR_BOX_W, ROW_H-2},
                    "Chop yield:", &p->pendingChopYield, 1, 20, p->editChop))
        p->editChop = !p->editChop;

    // Disable trade execution toggle (staged)
    GuiCheckBox((Rectangle){BR_X + PAD, BR_ROW4_Y + 4, ROW_H - 8, ROW_H - 8},
                "No Exchange", &p->pendingDisableExecuting);

    // Inflation toggle (staged)
    GuiCheckBox((Rectangle){BR_X + PAD, BR_ROW5_Y + 4, ROW_H - 8, ROW_H - 8},
                "Inflation", &p->pendingInflation);

    // Apply button — commit staged values to globals
    if (GuiButton((Rectangle){BR_X + PAD, BR_BTN_Y, BR_W - 2*PAD, BTN_H}, "Apply")) {
        g_wood_decay_rate  = p->pendingWoodDecay;
        g_chair_decay_rate = p->pendingChairDecay;
        g_chop_yield       = p->pendingChopYield;
        g_disable_executing_trade = p->pendingDisableExecuting;
        g_inflation_enabled = p->pendingInflation;
    }
}
