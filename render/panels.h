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
// Plot panels
// ---------------------------------------------------------------------------

// Price history: per-agent price expectations over time + average.
// showIndividualUtil: draw per-agent marginal buy utility lines.
void panel_price_history(const AgentValueHistory *avh, const AgentValueHistory *pvh,
                         const Agent *agents, int count, int marketId,
                         int px, int py, int pw, int ph,
                         Price equilibrium, bool showIndividualUtil);

// Supply & demand step curves with equilibrium marker.
void panel_supply_demand(const Agent *agents, int count, int marketId,
                         int px, int py, int pw, int ph);

// Money vs goods scatter plot.
void panel_wealth(const Agent *agents, int count, int marketId,
                  int px, int py, int pw, int ph);

// Agents sorted by max utility with buy/sell/price dots.
void panel_valuation_dist(const Agent *agents, int count, int marketId,
                           int px, int py, int pw, int ph, Price equilibrium);

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
