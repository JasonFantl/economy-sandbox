#ifndef INPUT_H
#define INPUT_H

#include "sim.h"
#include "render/render.h"
#include "render/controls.h"
#include "render/inspector.h"
#include "render/camera.h"
#include "walkthrough/walkthrough.h"

// F / Shift-F: double or halve simulation speed
void input_handle_speed(void);

// All input for walkthrough mode — returns true if a step change occurred
// (caller should reset history buffers on true)
bool input_handle_walkthrough(WalkthroughState *wt, SimContext *ctx);

// All input for free-play mode
void input_handle_freeplay(PanelState panels[NUM_PANELS], InfluencePanel *inf,
                            DecayRatePanel *decay, Inspector *inspector);

#endif
