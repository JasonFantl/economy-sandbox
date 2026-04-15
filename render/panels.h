#ifndef PANELS_H
#define PANELS_H

#include "econ/agent.h"
#include "econ/market.h"
#include "render/controls.h"
#include "render/render.h"
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Shared plot state (y-axis bounds, editable at runtime)
// ---------------------------------------------------------------------------
typedef struct { float xMax; float yMax; } PlotBounds;
extern PlotBounds g_bounds[PLOT_COUNT][MARKET_COUNT];  // defined in panels.c

// ---------------------------------------------------------------------------
// Y-range input box (editable yMax overlay on top of a plot)
// ---------------------------------------------------------------------------
typedef struct {
    bool editMode;
    char buf[12];
} YRangeBox;

// ---------------------------------------------------------------------------
// Wealth plot axis configuration (used by panel_wealth)
// ---------------------------------------------------------------------------
typedef enum {
    WEALTH_AXIS_MONEY      = 0,
    WEALTH_AXIS_WOOD_COUNT = 1,
    WEALTH_AXIS_WOOD_UTIL  = 2,
    WEALTH_AXIS_COUNT      = 3
} WealthAxis;

typedef struct {
    WealthAxis xAxis;
    WealthAxis yAxis;
    float      axisMax[WEALTH_AXIS_COUNT];   // per-axis range max
    bool       xEditMode;                    // dropdown open state for X axis
    bool       yEditMode;                    // dropdown open state for Y axis
    char       dropdownText[64];             // semicolon-separated option labels for GuiDropdownBox
} WealthAxisConfig;

// ---------------------------------------------------------------------------
// Plot panels
// ---------------------------------------------------------------------------

// Price history: per-agent price expectations + indifference prices over time.
// ybox: if non-NULL, draws an editable Y-max text box at the top-left of the plot.
// speedh: if non-NULL, draws speed-tinted background and change markers.
void panel_price_history(const AgentValueHistory *avh, const AgentValueHistory *pvh,
                         const Agent *agents, int count, int marketId,
                         int px, int py, int pw, int ph,
                         YRangeBox *ybox, const SpeedHistory *speedh);

// Supply & demand step curves with equilibrium marker.
void panel_supply_demand(const Agent *agents, int count, int marketId,
                         int px, int py, int pw, int ph);

// Money vs goods scatter plot.
// cfg: if non-NULL, axis selector buttons are drawn and cfg controls which
//      values map to X/Y; if NULL, defaults to Wood Count (x) vs Money (y).
void panel_wealth(const Agent *agents, int count, int marketId,
                  int px, int py, int pw, int ph, WealthAxisConfig *cfg);

// Agents sorted by max utility with buy/sell/price dots.
// ybox: if non-NULL, draws an editable Y-max text box at the top-left of the plot.
void panel_valuation_dist(const Agent *agents, int count, int marketId,
                           int px, int py, int pw, int ph, YRangeBox *ybox);

// Goods-count time series per agent + average.
void panel_goods_history(const AgentValueHistory *gvh, int marketId,
                         int px, int py, int pw, int ph);

// Money time series per agent + average.
void panel_money_history(const AgentValueHistory *mvh, int marketId,
                         int px, int py, int pw, int ph);

// ---------------------------------------------------------------------------
// Control panels (update + render combined; return true if input consumed)
// ---------------------------------------------------------------------------

void panel_ctrl_influence_render(InfluencePanel *p, Agent *agents, int count, int px, int py);
void panel_ctrl_decay_render(DecayRatePanel *p, int px, int py);

#endif
