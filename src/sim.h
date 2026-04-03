#ifndef SIM_H
#define SIM_H

#include "agent.h"
#include "market.h"
#include <stdbool.h>

#define NUM_AGENTS 120

typedef struct {
    bool  paused;
    int   steps;     // sim steps per frame (speed multiplier)
    int   count;     // active agent count
    float worldW, worldH;
    float priceTimer;
    Agent             agents[NUM_AGENTS];
    AgentValueHistory avh[MARKET_COUNT];
    AgentValueHistory pvh[MARKET_COUNT];
    AgentValueHistory gvh[MARKET_COUNT];
} SimState;

extern SimState g_sim;

#endif
