#ifndef WT_PANELS_H
#define WT_PANELS_H

#include "econ/agent.h"
#include "render/panels.h"
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Walkthrough-specific panels — used only within walkthrough scenes.
// Add new scene-specific controls here rather than in render/controls.h.
// ---------------------------------------------------------------------------

// Flags controlling which rows appear in the influence panel.
// Steps expose rows cumulatively as the walkthrough progresses.
#define WT_INF_WOOD_VALUE  (1 << 0)   // delta: shift every agent's wood valuation
#define WT_INF_WOOD_COUNT  (1 << 1)   // delta: give / take wood from every agent
#define WT_INF_LEISURE     (1 << 2)   // setter: set every agent's leisure utility
#define WT_INF_MARKET_SEL  (1 << 3)   // show market selector button on delta rows
#define WT_INF_INFLATION   (1 << 4)   // checkbox + Set: toggle g_inflation_enabled
#define WT_INF_MONEY       (1 << 5)   // delta: give / take money from every agent

typedef struct {
    bool  expanded;
    bool  restart_requested;  // set by render; cleared by caller after handling
    // Wood value row (delta)
    Utility woodUtilityDelta;
    bool    editWoodValue;
    char    bufWoodValue[16];
    // Wood count row (delta)
    int     woodCountDelta;
    bool    editWoodCount;
    char    bufWoodCount[16];
    // Leisure row (setter)
    Utility leisureValue;
    bool    editLeisure;
    char    bufLeisure[16];
    Utility lastLeisure;  // last global value seen; used to detect external changes
    // Inflation toggle
    bool    pendingInflation;
    bool    lastInflation;  // last global value seen; used to detect external changes
    // Money row (delta)
    Price   moneyDelta;
    bool    editMoney;
    char    bufMoney[16];
    // Market selector state (used when WT_INF_MARKET_SEL is set)
    int     valueMarket;  // 0=Wood, 1=Chair
    int     goodsMarket;  // 0=Wood, 1=Chair
} WtInfluencePanel;

void wt_influence_panel_init(WtInfluencePanel *p);

// Render at screen position (px, py=32).  flags selects which rows to show.
void wt_influence_panel_render(WtInfluencePanel *p, Agent *agents, int agentCount,
                                int flags, int px);

// ---------------------------------------------------------------------------
// Environment panel — tune world parameters that affect all agents
// ---------------------------------------------------------------------------

#define WT_ENV_WOOD_DECAY       (1 << 0)   // decay rate row (wood only)
#define WT_ENV_CHOP_YIELD       (1 << 1)   // wood gained per chop action
#define WT_ENV_BUILD_COST       (1 << 2)   // wood units required to build one chair
#define WT_ENV_DECAY_MARKET_SEL (1 << 3)   // add Wood/Chair toggle to decay row

typedef struct {
    bool  expanded;
    float decayRate;          // current value in text box
    bool  editDecay;
    char  bufDecay[16];
    float lastDecayRate;      // last global value seen; used to detect external changes
    int   decayMarket;        // 0=Wood, 1=Chair — which decay global is shown
    int   chopYield;
    bool  editChopYield;
    char  bufChopYield[16];
    int   lastChopYield;      // last global value seen; used to detect external changes
    // Build cost row
    int   woodPerChair;
    bool  editWoodPerChair;
    char  bufWoodPerChair[16];
    int   lastWoodPerChair;   // last global value seen; used to detect external changes
} WtEnvironmentPanel;

void wt_environment_panel_init(WtEnvironmentPanel *p);

// Render at screen position (px, py=32).  flags selects which rows to show.
void wt_environment_panel_render(WtEnvironmentPanel *p, int flags, int px);

// ---------------------------------------------------------------------------
// Wealth plot axis configuration init
// ---------------------------------------------------------------------------

// Initialise with defaults: X = Wood Count, Y = Money
void wt_wealth_config_init(WealthAxisConfig *c);

#endif
