#ifndef CONTROLS_H
#define CONTROLS_H

#include "agent.h"
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Influence panel — inject exogenous shocks to money, goods, or valuations
// ---------------------------------------------------------------------------

typedef enum { INF_EDIT_NONE = 0, INF_EDIT_NUM_AGENTS, INF_EDIT_MONEY, INF_EDIT_VALUATION, INF_EDIT_GOODS } InfluenceEditField;

typedef struct {
    int                numAgents;    // how many agents to affect
    float              moneyDelta;   // change in money (market-independent)
    float              valuationDelta; // change in maxUtility for selected market
    int                goodsDelta;   // change in goods count for selected market
    MarketId           marketId;     // which market valuationDelta and goodsDelta apply to
    InfluenceEditField editing;
    char               buf[16];
    int                bufLen;
} InfluencePanel;

void influence_panel_init(InfluencePanel *p);
bool influence_panel_update(InfluencePanel *p, Agent *agents, int agentCount);
void influence_panel_render(const InfluencePanel *p);

// ---------------------------------------------------------------------------
// Decay rate panel — edit per-unit, per-second decay rates for each good
// ---------------------------------------------------------------------------

typedef enum { DECAY_EDIT_NONE = 0, DECAY_EDIT_WOOD, DECAY_EDIT_CHAIR, DECAY_EDIT_CHOP_YIELD } DecayEditField;

typedef struct {
    DecayEditField editing;
    char           buf[16];
    int            bufLen;
} DecayRatePanel;

void decay_rate_panel_init(DecayRatePanel *p);
bool decay_rate_panel_update(DecayRatePanel *p);
void decay_rate_panel_render(const DecayRatePanel *p);

#endif
