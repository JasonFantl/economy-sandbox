#ifndef RENDER_H
#define RENDER_H

#include "agent.h"
#include "market.h"
#include "assets.h"
#include "world.h"
#include "tileset.h"
#include <stdbool.h>

#define SCREEN_W        1200
#define SCREEN_H         700
// World viewport: top portion used for the tile map
#define WORLD_VIEW_H     420   // pixels of screen dedicated to the tile map
// Plot area: bottom portion
#define PLOT_MARGIN_L     40
#define PLOT_MARGIN_R     30
#define PLOT_MARGIN_T     12
#define PLOT_MARGIN_B     15
#define PANEL_GAP         30
#define PANEL_ROW_GAP      4
#define STRIP_H           16
#define NUM_PANELS         4

// Agent sprite display size — half a tile (16px) so agents fit neatly on tile grid
#define AGENT_DISP  ((int)(WORKER_FRAME_W))

typedef enum {
    PLOT_WEALTH                 = 0,  // money vs goods scatter
    PLOT_VALUATION_DISTRIBUTION = 1,  // agents sorted by max utility with price dots
    PLOT_PRICE_HISTORY          = 2,  // price expectations and personal valuations over time
    PLOT_GOODS_HISTORY          = 3,  // goods count per agent over time
    PLOT_SUPPLY_DEMAND          = 4,  // supply and demand step curves with equilibrium
    PLOT_COUNT                  = 5
} PlotType;

typedef struct {
    PlotType plotType;
    int      marketId;
} PanelState;

// Render the top-down tile world and agents
// camX/camY: world coordinate at the centre of the viewport; camZoom: scale factor
void render_world(const WorldMap *map, const TileAtlas *tiles,
                  const Agent *agents, int count,
                  bool paused, int simSteps,
                  const Assets *assets,
                  float camX, float camY, float camZoom);

// Render the bottom plot area
void render_plot(const AgentValueHistory avh[MARKET_COUNT],
                 const AgentValueHistory pvh[MARKET_COUNT],
                 const AgentValueHistory gvh[MARKET_COUNT],
                 const Agent *agents, int agentCount,
                 PanelState panels[NUM_PANELS]);

bool panel_handle_click(PanelState panels[NUM_PANELS]);
bool panel_handle_bounds_keyboard(void);

#endif
