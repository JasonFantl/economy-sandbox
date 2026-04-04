#ifndef NAV_H
#define NAV_H

#include <stdbool.h>
#include "econ/agent.h"
#include "world/world.h"

// Work-site pixel positions (assigned by agents_nav_init; useful for rendering markers)
extern float g_chop_site_px, g_chop_site_py;
extern float g_build_site_px, g_build_site_py;

// Build the walkable nav graph from the tile map.  Must be called before agents_init.
void agents_nav_init(const WorldMap *map);

// Assign a new movement target (and BFS path) to agents[agentIdx].
// When a trade meeting is arranged, the partner also gets their path assigned here.
void agents_pick_new_target(Agent *agents, int agentIdx, int agentCount,
                             float worldWidth, float worldHeight);

// Place (px, py) at a random walkable tile centre.
// Returns true if nav data is available, false otherwise.
bool nav_random_position(float *px, float *py);

#endif
