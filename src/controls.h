#ifndef CONTROLS_H
#define CONTROLS_H

#include "agent.h"
#include <stdbool.h>

typedef enum { INF_EDIT_NONE = 0, INF_EDIT_N, INF_EDIT_MONEY, INF_EDIT_F, INF_EDIT_GOODS } InfEditField;

typedef struct {
    int          n;          // agents to influence
    float        deltaMoney; // delta money (market-independent)
    float        f;          // delta personal value
    int          deltaGoods; // delta goods
    MarketId     marketId;   // which market to influence (for f and deltaGoods)
    InfEditField editing;
    char         buf[16];
    int          bufLen;
} InfluencePanel;

void influence_panel_init(InfluencePanel *p);
bool influence_panel_update(InfluencePanel *p, Agent *agents, int agentCount);
void influence_panel_render(const InfluencePanel *p);

typedef enum { BR_EDIT_NONE = 0, BR_EDIT_WOOD, BR_EDIT_CHAIR } BrEditField;

typedef struct {
    BrEditField editing;
    char        buf[16];
    int         bufLen;
} BreakRatePanel;

void break_rate_panel_init(BreakRatePanel *p);
// Handle input. Returns true if a mouse click was consumed.
bool break_rate_panel_update(BreakRatePanel *p);
void break_rate_panel_render(const BreakRatePanel *p);

#endif
