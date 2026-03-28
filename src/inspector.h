#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "agent.h"
#include <stdbool.h>

typedef enum {
    EDIT_NONE        = -1,
    EDIT_BASE_VALUE  =  0,  // basePersonalValue (S)
    EDIT_EXPECTED_VALUE =  1,
} EditField;

typedef struct {
    int       selectedId;    // -1 = nothing selected
    EditField editField;     // which field is being edited, or EDIT_NONE
    char      inputBuf[32];
    int       inputLen;
} Inspector;

void inspector_init(Inspector *ins);

// Process mouse clicks and keyboard input.
// Returns true if a mouse click was consumed by the inspector.
bool inspector_update(Inspector *ins, Agent *agents, int agentCount);

// Draw the panel (no-op if nothing selected).
void inspector_render(const Inspector *ins, const Agent *agents);

#endif
