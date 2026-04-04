#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "econ/agent.h"
#include <stdbool.h>

typedef struct {
    int  selectedId;
    // raygui ValueBoxFloat edit state for the four editable fields
    bool editWoodUtil;
    char bufWoodUtil[16];
    bool editWoodPrice;
    char bufWoodPrice[16];
    bool editChairUtil;
    char bufChairUtil[16];
    bool editChairPrice;
    char bufChairPrice[16];
} Inspector;

void inspector_init(Inspector *ins);

// World-space agent click detection only (field editing handled in render)
bool inspector_update(Inspector *ins, Agent *agents, int agentCount,
                      float camX, float camY, float camZoom);

// Render panel; also handles field edits and close button (immediate-mode)
void inspector_render(Inspector *ins, Agent *agents,
                      float camX, float camY, float camZoom);

#endif
