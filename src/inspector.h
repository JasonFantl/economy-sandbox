#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "agent.h"
#include <stdbool.h>

typedef enum {
    EDIT_NONE      = -1,
    EDIT_WOOD_S    =  0,
    EDIT_WOOD_EMV  =  1,
    EDIT_CHAIR_S   =  2,
    EDIT_CHAIR_EMV =  3,
} EditField;

typedef struct {
    int       selectedId;
    EditField editField;
    char      inputBuf[32];
    int       inputLen;
} Inspector;

void inspector_init(Inspector *ins);
bool inspector_update(Inspector *ins, Agent *agents, int agentCount);
void inspector_render(const Inspector *ins, const Agent *agents);

#endif
