#ifndef WALKTHROUGH_H
#define WALKTHROUGH_H

#include "sim.h"
#include "render/controls.h"
#include "render/panels.h"
#include "walkthrough/wt_panels.h"
#include <stdbool.h>

// WTHROUGH_NAV_H: height of the walkthrough navigation bar at the top of the screen
#define WTHROUGH_NAV_H 36

// ---------------------------------------------------------------------------
// SimContext — sim data + walkthrough UI panels, passed to step render fns
// ---------------------------------------------------------------------------
typedef struct {
    SimState          *sim;
    InfluencePanel    *inf;
    DecayRatePanel    *decay;
    WtInfluencePanel  *wt_inf;
    WtEnvironmentPanel *wt_env;
    WealthAxisConfig  *wt_wealth;
} SimContext;

// ---------------------------------------------------------------------------
// Step render callback
// ---------------------------------------------------------------------------
typedef void (*RenderPanelsFn)(const SimContext *ctx,
                                int plotX, int plotY, int plotW, int plotH);

// Scene init callback — sets globals and ctx->count; agents_init called after
typedef void (*SceneInitFn)(SimContext *ctx);

// Step init callback — sets feature flags only (no agent reinit)
typedef void (*StepInitFn)(void);

// ---------------------------------------------------------------------------
// Step definition
// ---------------------------------------------------------------------------
typedef struct {
    const char    *title;
    const char    *text;
    StepInitFn     init;          // sets flags for this step (no agent reinit)
    RenderPanelsFn render_panels;
} StepDef;

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------
#define MAX_STEPS_PER_SCENE 8
#define MAX_SCENES          8

typedef struct {
    const char  *title;
    SceneInitFn  init;       // called on scene entry; sets globals + ctx->count
    int          stepCount;
    StepDef      steps[MAX_STEPS_PER_SCENE];
} SceneDef;

// ---------------------------------------------------------------------------
// Walkthrough state
// ---------------------------------------------------------------------------
typedef struct {
    int  scene, step;
    bool active;        // false = free-play mode
    bool scene_changed; // set true when scene changes; cleared by caller
    bool popup_active;  // true = step intro popup is showing
    bool seen[MAX_SCENES][MAX_STEPS_PER_SCENE]; // popup shown once per step
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

void walkthrough_render_overlay(WalkthroughState *wt, SimContext *ctx);

void walkthrough_apply(WalkthroughState *wt, SimContext *ctx);

// Get the current step definition (for accessing render_panels callback)
const StepDef *walkthrough_current_step(const WalkthroughState *wt);

#endif
