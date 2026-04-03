#ifndef WALKTHROUGH_H
#define WALKTHROUGH_H

#include "agent.h"
#include "market.h"
#include "controls.h"
#include "panels.h"
#include <stdbool.h>

// WTHROUGH_NAV_H: height of the walkthrough navigation bar at the top of the screen
#define WTHROUGH_NAV_H 36

// ---------------------------------------------------------------------------
// SimContext — shared data bundle passed to step render functions
// ---------------------------------------------------------------------------
typedef struct {
    Agent             *agents;
    int                count;
    const AgentValueHistory *avh;  // [MARKET_COUNT]
    const AgentValueHistory *pvh;  // [MARKET_COUNT]
    const AgentValueHistory *gvh;  // [MARKET_COUNT]
    InfluencePanel    *inf;
    DecayRatePanel    *decay;
    float              worldW, worldH;
} SimContext;

// ---------------------------------------------------------------------------
// Step render callback
// ---------------------------------------------------------------------------
typedef void (*RenderPanelsFn)(const SimContext *ctx,
                                int plotX, int plotY, int plotW, int plotH);

// ---------------------------------------------------------------------------
// Step definition
// ---------------------------------------------------------------------------
typedef struct {
    // Simulation config applied on step entry
    int   numAgents;
    bool  diminishingReturns;
    bool  debtAllowed;
    bool  productionEnabled;
    bool  decayEnabled;
    bool  leisureEnabled;
    bool  twoGoods;
    float baseValueMin, baseValueMax;

    // UI
    const char    *title;
    const char    *text;
    RenderPanelsFn render_panels;
} StepDef;

// ---------------------------------------------------------------------------
// Scene and Chapter
// ---------------------------------------------------------------------------
#define MAX_STEPS_PER_SCENE    8
#define MAX_SCENES_PER_CHAPTER 4
#define MAX_CHAPTERS           3

typedef struct {
    const char *title;
    int         stepCount;
    StepDef     steps[MAX_STEPS_PER_SCENE];
} SceneDef;

typedef struct {
    const char *title;
    int         sceneCount;
    SceneDef    scenes[MAX_SCENES_PER_CHAPTER];
} ChapterDef;

// ---------------------------------------------------------------------------
// Walkthrough state
// ---------------------------------------------------------------------------
typedef struct {
    int  chapter, scene, step;
    bool active;  // false = free-play mode
} WalkthroughState;

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

void walkthrough_init(WalkthroughState *wt, SimContext *ctx);

bool walkthrough_next_step(WalkthroughState *wt, SimContext *ctx);
bool walkthrough_prev_step(WalkthroughState *wt, SimContext *ctx);

void walkthrough_restart(WalkthroughState *wt, SimContext *ctx);

void walkthrough_exit(WalkthroughState *wt);

bool walkthrough_handle_input(WalkthroughState *wt, SimContext *ctx);

void walkthrough_render_overlay(const WalkthroughState *wt);

void walkthrough_apply(WalkthroughState *wt, SimContext *ctx);

// Get the current step definition (for accessing render_panels callback)
const StepDef *walkthrough_current_step(const WalkthroughState *wt);

#endif
