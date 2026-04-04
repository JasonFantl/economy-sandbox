#ifndef CONTROLS_H
#define CONTROLS_H

#include "econ/agent.h"
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Influence panel — inject exogenous shocks to money, goods, or valuations
// ---------------------------------------------------------------------------

typedef struct {
    bool     expanded;
    int      numAgents;
    float    moneyDelta;
    float    valuationDelta;
    int      goodsDelta;
    MarketId marketId;
    // raygui ValueBoxFloat edit state
    bool     editMoney;
    char     bufMoney[16];
    bool     editValuation;
    char     bufValuation[16];
    bool     editN;
    bool     editGoods;
} InfluencePanel;

void influence_panel_init(InfluencePanel *p);
// Render + handle interaction (immediate-mode: input and draw combined)
void influence_panel_render(InfluencePanel *p, Agent *agents, int agentCount);

// ---------------------------------------------------------------------------
// Decay rate panel — edit per-unit, per-second decay rates for each good
// ---------------------------------------------------------------------------

typedef struct {
    bool expanded;
    bool editWood;
    char bufWood[16];
    bool editChair;
    char bufChair[16];
    bool editChop;
} DecayRatePanel;

void decay_rate_panel_init(DecayRatePanel *p);
void decay_rate_panel_render(DecayRatePanel *p);

#endif
