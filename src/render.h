#ifndef RENDER_H
#define RENDER_H

#include "agent.h"
#include "market.h"

#define SCREEN_W     1200
#define SCREEN_H     700
#define WORLD_WIDTH  1160.0f
#define GROUND_Y     390
#define WORLD_AREA_H 450  // world view occupies top 450px; plot below
#define PLOT_MARGIN_L 60
#define PLOT_MARGIN_R 20
#define PLOT_MARGIN_T 20
#define PLOT_MARGIN_B 35

void render_world(const Agent *agents, int count);
void render_plot(const PriceHistory *ph, const Agent *agents, int agentCount);

#endif
