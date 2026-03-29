#ifndef RENDER_H
#define RENDER_H

#include "agent.h"
#include "market.h"
#include "assets.h"
#include <stdbool.h>

#define SCREEN_W      1200
#define SCREEN_H      700
#define WORLD_WIDTH   1160.0f
#define GROUND_Y      390
#define WORLD_AREA_H  450
#define SPRITE_SCALE  2.0f   // display scale: 32px sprite cell → 64px on screen
#define PLOT_MARGIN_L  40
#define PLOT_MARGIN_R  30
#define PLOT_MARGIN_T  20
#define PLOT_MARGIN_B  30
#define PANEL_GAP      30

typedef enum {
    PLOT_WEALTH      = 0,  // money vs goods scatter
    PLOT_EMV_HISTORY = 1,  // expected market values over time
    PLOT_COUNT       = 2
} PlotType;

void render_world(const Agent *agents, int count, bool paused, int simSteps,
                  const Assets *assets);
void render_plot(const AgentValueHistory *avh, const Agent *agents, int agentCount,
                 PlotType leftPlot, PlotType rightPlot);

// Check for clicks on the panel header strips; cycle the plot type if hit.
// Returns true if a click was consumed.
bool plot_cycle_click(PlotType *left, PlotType *right);

#endif
