#ifndef CONTROLS_H
#define CONTROLS_H

#include "agent.h"
#include <stdbool.h>

typedef enum { INF_EDIT_NONE = 0, INF_EDIT_N, INF_EDIT_F } InfEditField;

typedef struct {
    int          n;        // agents to influence
    float        f;        // delta personal value (can be negative)
    InfEditField editing;
    char         buf[16];
    int          bufLen;
} InfluencePanel;

void influence_panel_init(InfluencePanel *p);
// Handle input; apply influence if button clicked. Returns true if mouse click consumed.
bool influence_panel_update(InfluencePanel *p, Agent *agents, int agentCount);
void influence_panel_render(const InfluencePanel *p);

#endif
