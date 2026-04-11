#ifndef SIM_H
#define SIM_H

#include "econ/agent.h"
#include "econ/market.h"
#include <stdbool.h>

#define NUM_AGENTS 120

typedef struct {
    bool  paused;
    int   ticks_per_frame;     // sim steps per frame (speed multiplier)
    int   count;     // active agent count
    float worldW, worldH;
    int   priceTick;
    Agent             agents[NUM_AGENTS];
    AgentValueHistory avh[MARKET_COUNT];
    AgentValueHistory pvh[MARKET_COUNT];
    AgentValueHistory gvh[MARKET_COUNT];
    AgentValueHistory mvh;  // money history (not market-specific)
} SimState;

extern SimState g_simulation;

void sim_update(void);
void sim_restart(void);

#endif
