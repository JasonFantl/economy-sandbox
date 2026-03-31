#ifndef RENDER_H
#define RENDER_H

#include "agent.h"
#include "market.h"
#include "assets.h"
#include <stdbool.h>

#define SCREEN_W       1200
#define SCREEN_H        700
#define WORLD_WIDTH    1160.0f
#define GROUND_Y        235
#define WORLD_AREA_H    260
#define SPRITE_SCALE    2.0f
#define PLOT_MARGIN_L    40
#define PLOT_MARGIN_R    30
#define PLOT_MARGIN_T    12
#define PLOT_MARGIN_B    15
#define PANEL_GAP        30
#define PANEL_ROW_GAP     4
#define STRIP_H          16
#define NUM_PANELS        4

typedef enum {
    PLOT_WEALTH                 = 0,  // money vs goods scatter
    PLOT_VALUATION_DISTRIBUTION = 1,  // agents sorted by max utility with price dots
    PLOT_PRICE_HISTORY          = 2,  // price expectations and personal valuations over time
    PLOT_GOODS_HISTORY          = 3,  // goods count per agent over time
    PLOT_COUNT                  = 4
} PlotType;

// State for one plot panel: which plot type and which market to display
typedef struct {
    PlotType plotType;
    int      marketId;   // MarketId cast to int
} PanelState;

void render_world(const Agent *agents, int count, bool paused, int simSteps,
                  const Assets *assets);

// avh/pvh/gvh are arrays of MARKET_COUNT histories; panels is NUM_PANELS entries (TL,TR,BL,BR)
void render_plot(const AgentValueHistory avh[MARKET_COUNT],
                 const AgentValueHistory pvh[MARKET_COUNT],
                 const AgentValueHistory gvh[MARKET_COUNT],
                 const Agent *agents, int agentCount,
                 PanelState panels[NUM_PANELS]);

// Check for clicks on panel strips; cycle plot type/market or start bounds edit.
// Returns true if a click was consumed.
bool panel_handle_click(PanelState panels[NUM_PANELS]);

// Process keyboard input for the active bounds editor.
// Returns true if keyboard input was consumed.
bool panel_handle_bounds_keyboard(void);

#endif
