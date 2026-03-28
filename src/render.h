#ifndef RENDER_H
#define RENDER_H

#include "agent.h"
#include "market.h"
#include <stdbool.h>

#define SCREEN_W      1200
#define SCREEN_H      700
#define WORLD_WIDTH   1160.0f
#define GROUND_Y      390
#define WORLD_AREA_H  450
#define PLOT_MARGIN_L  40
#define PLOT_MARGIN_R  30
#define PLOT_MARGIN_T  20
#define PLOT_MARGIN_B  30
#define PANEL_GAP      30

void render_world(const Agent *agents, int count, bool paused, float timeScale);
void render_plot(const PriceHistory *ph, const Agent *agents, int agentCount);

#endif
