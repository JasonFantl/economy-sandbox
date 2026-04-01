#ifndef AGENT_H
#define AGENT_H

#include "agent_types.h"
#include "econ.h"
#include "nav.h"

// ---------------------------------------------------------------------------
// agent.c — world/body orchestration
// ---------------------------------------------------------------------------

// Scatter agents at random walkable positions and pick initial targets.
void agents_init(Agent *agents, int count, float worldWidth, float worldHeight);

// Advance all agents by dt seconds (movement, animation, economic update).
void agents_update(Agent *agents, int count, float dt);

#endif
