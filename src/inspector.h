#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "agent.h"
#include <stdbool.h>

typedef enum {
    EDIT_NONE         = -1,
    EDIT_WOOD_UTILITY =  0,
    EDIT_WOOD_PRICE   =  1,
    EDIT_CHAIR_UTILITY =  2,
    EDIT_CHAIR_PRICE  =  3,
} EditField;

typedef struct {
    int       selectedId;
    EditField editField;
    char      inputBuf[32];
    int       inputLen;
} Inspector;

void inspector_init(Inspector *ins);
// camX/camY: world coord at viewport centre; camZoom: current zoom
bool inspector_update(Inspector *ins, Agent *agents, int agentCount,
                      float camX, float camY, float camZoom);
void inspector_render(const Inspector *ins, const Agent *agents,
                      float camX, float camY, float camZoom);

#endif
