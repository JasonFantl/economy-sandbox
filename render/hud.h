#ifndef HUD_H
#define HUD_H

#include "sim.h"
#include "render/render.h"
#include "render/controls.h"
#include "render/inspector.h"
#include "world/tileset.h"
#include "walkthrough/wt_panels.h"
#include "walkthrough/walkthrough.h"

// Shared panel state — exposed so main can wire ctx->inf / ctx->decay
extern InfluencePanel g_influence;
extern DecayRatePanel g_decay_rates;
extern WtInfluencePanel  g_wt_influence;
extern WtEnvironmentPanel g_wt_environment;
extern WealthAxisConfig  g_wt_wealth;

// Sprite/worker assets — exposed so main can pass to render_world
extern Assets g_assets;

// Initialise all panels and load assets
void hud_init(void);

// Unload assets
void hud_unload(void);

// Mode-specific frame: handle input then render panels (call inside BeginDrawing)
void hud_freeplay_frame(void);
void hud_walkthrough_frame(WalkthroughState *wt, SimContext *ctx);

#endif
