#ifndef AGENT_RENDER_H
#define AGENT_RENDER_H

#include "agent_types.h"
#include "assets.h"

// Draw a single agent sprite at its current world position.
// Must be called inside a BeginMode2D / EndMode2D block.
void draw_agent(const Agent *a, const Assets *assets);

#endif
