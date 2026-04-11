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
// Geometry — panel width matches render/controls.h panels (210px) so it can
// sit at x=10 alongside the decay panel at x=228 without overlapping.
// Content width = 210 - 2*8 = 194.
//
// Setter row:  [Label 54] [gap 4] [TextBox 76] [gap 4] [Apply 56]        = 194
// Delta row:   [Label 44] [gap 4] [+/- 24] [gap 2] [TextBox 64] [gap 4] [Apply 52] = 194
//              Delta rows have a "+/-" indicator in front of the text box.
// ---------------------------------------------------------------------------

#define WT_W      210
#define WT_Y      32
#define WT_PAD    8
#define WT_ROW_H  24
#define WT_BTN_H  24
#define WT_SEP    4
#define WT_HDR_H  24

// Setter row geometry
#define WT_LBL_W   54
#define WT_BOX_W   76
#define WT_APL_W   56
#define WT_LBL_DX  (WT_PAD)
#define WT_BOX_DX  (WT_PAD + WT_LBL_W + 4)
#define WT_APL_DX  (WT_PAD + WT_LBL_W + 4 + WT_BOX_W + 4)

// Delta row geometry (same as setter row, label slightly narrower)
#define WT_DLBL_W  44
#define WT_DBOX_W  90
#define WT_DAPL_W  52
#define WT_DLBL_DX (WT_PAD)
#define WT_DBOX_DX (WT_PAD + WT_DLBL_W + 4)
#define WT_DAPL_DX (WT_PAD + WT_DLBL_W + 4 + WT_DBOX_W + 4)

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
    snprintf(p->bufWoodValue, sizeof(p->bufWoodValue), "%.1f", p->woodValueDelta);
    snprintf(p->bufWoodCount, sizeof(p->bufWoodCount), "%d",   p->woodCountDelta);
    snprintf(p->bufLeisure,   sizeof(p->bufLeisure),   "%.1f", p->leisureValue);
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

    int contentH = rows * (WT_ROW_H + WT_SEP) + WT_SEP + WT_BTN_H + WT_PAD;
    int contentY = WT_Y + WT_HDR_H;
    DrawRectangle(px, contentY, WT_W, contentH, WT_BG);
    DrawRectangleLines(px, contentY, WT_W, contentH, WT_BORDER);

    int rowY = contentY + WT_SEP;

    // --- Wood value row (delta) ---
    if (flags & WT_INF_WOOD_VALUE) {
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
        rowY += WT_ROW_H + WT_SEP;
    }

    // --- Wood count row (delta) ---
    if (flags & WT_INF_WOOD_COUNT) {
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
        if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set"))
            agents_set_leisure(agents, agentCount, p->leisureValue);
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
    p->expanded      = false;
    p->woodDecayRate    = 0.003f;
    p->editWoodDecay    = false;
    p->lastWoodDecayRate = p->woodDecayRate;
    p->chopYield        = 1;
    p->editChopYield    = false;
    p->lastChopYield    = p->chopYield;
    snprintf(p->bufWoodDecay,  sizeof(p->bufWoodDecay),  "%.4f", p->woodDecayRate);
    snprintf(p->bufChopYield,  sizeof(p->bufChopYield),  "%d",   p->chopYield);
}

void wt_environment_panel_render(WtEnvironmentPanel *p, int flags, int px) {
    if (GuiButton((Rectangle){px, WT_Y, WT_W, WT_HDR_H},
                  p->expanded ? "v  Environment" : ">  Environment"))
        p->expanded = !p->expanded;

    if (!p->expanded) return;

    int rows = 0;
    if (flags & WT_ENV_WOOD_DECAY) rows++;
    if (flags & WT_ENV_CHOP_YIELD) rows++;

    int contentH = rows * (WT_ROW_H + WT_SEP) + WT_PAD;
    int contentY = WT_Y + WT_HDR_H;
    DrawRectangle(px, contentY, WT_W, contentH, WT_BG);
    DrawRectangleLines(px, contentY, WT_W, contentH, WT_BORDER);

    int rowY = contentY + WT_SEP;

    // --- Wood decay rate row ---
    if (flags & WT_ENV_WOOD_DECAY) {
        // Sync only when the global changed externally since last frame
        if (g_wood_decay_rate != p->lastWoodDecayRate) {
            p->woodDecayRate     = g_wood_decay_rate;
            p->lastWoodDecayRate = g_wood_decay_rate;
            snprintf(p->bufWoodDecay, sizeof(p->bufWoodDecay), "%.4f", p->woodDecayRate);
        }
        GuiLabel((Rectangle){px + WT_LBL_DX, rowY, WT_LBL_W, WT_ROW_H - 2},
                 "Decay:");
        if (GuiTextBox((Rectangle){px + WT_BOX_DX, rowY, WT_BOX_W, WT_ROW_H - 2},
                       p->bufWoodDecay, (int)sizeof(p->bufWoodDecay), p->editWoodDecay)) {
            p->editWoodDecay = !p->editWoodDecay;
            if (!p->editWoodDecay)
                p->woodDecayRate = strtof(p->bufWoodDecay, NULL);
        }
        if (GuiButton((Rectangle){px + WT_APL_DX, rowY, WT_APL_W, WT_BTN_H}, "Set"))
            g_wood_decay_rate = p->woodDecayRate;
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

    (void)rowY;
}
