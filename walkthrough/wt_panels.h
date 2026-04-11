#ifndef WT_PANELS_H
#define WT_PANELS_H

#include "econ/agent.h"
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

typedef struct {
    bool  expanded;
    bool  restart_requested;  // set by render; cleared by caller after handling
    // Wood value row (delta)
    float woodValueDelta;
    bool  editWoodValue;
    char  bufWoodValue[16];
    // Wood count row (delta)
    int   woodCountDelta;
    bool  editWoodCount;
    char  bufWoodCount[16];
    // Leisure row (setter)
    float leisureValue;
    bool  editLeisure;
    char  bufLeisure[16];
    float lastLeisure;   // last global value seen; used to detect external changes
} WtInfluencePanel;

void wt_influence_panel_init(WtInfluencePanel *p);

// Render at screen position (px, py=32).  flags selects which rows to show.
void wt_influence_panel_render(WtInfluencePanel *p, Agent *agents, int agentCount,
                                int flags, int px);

// ---------------------------------------------------------------------------
// Environment panel — tune world parameters that affect all agents
// ---------------------------------------------------------------------------

#define WT_ENV_WOOD_DECAY  (1 << 0)   // wood decay rate
#define WT_ENV_CHOP_YIELD  (1 << 1)   // wood gained per chop action

typedef struct {
    bool  expanded;
    float woodDecayRate;
    bool  editWoodDecay;
    char  bufWoodDecay[16];
    float lastWoodDecayRate;  // last global value seen; used to detect external changes
    int   chopYield;
    bool  editChopYield;
    char  bufChopYield[16];
    int   lastChopYield;      // last global value seen; used to detect external changes
} WtEnvironmentPanel;

void wt_environment_panel_init(WtEnvironmentPanel *p);

// Render at screen position (px, py=32).  flags selects which rows to show.
void wt_environment_panel_render(WtEnvironmentPanel *p, int flags, int px);

#endif
