#include "inspector.h"
#include "render.h"   // GROUND_Y, WORLD_AREA_H
#include "assets.h"   // SPRITE_FRAME_SIZE
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Panel geometry (fixed position, top-right of world area)
#define INS_X    840
#define INS_Y     40
#define INS_W    330
#define HDR_H     28
#define ROW_H     26
#define SEP_H      4
#define PAD        6
// 6 rows + 2 separators + header + top/bottom padding
#define INS_H    (HDR_H + 2*PAD + 6*ROW_H + 2*SEP_H)

// Row start positions (relative to screen)
#define ROW_Y_ID     (INS_Y + HDR_H + PAD)
#define ROW_Y_STATUS (ROW_Y_ID     + ROW_H)
#define ROW_Y_PV     (ROW_Y_STATUS + ROW_H + SEP_H)
#define ROW_Y_EMV    (ROW_Y_PV     + ROW_H)
#define ROW_Y_TIMER  (ROW_Y_EMV    + ROW_H + SEP_H)
#define ROW_Y_TARGET (ROW_Y_TIMER  + ROW_H)

// ---- helpers ---------------------------------------------------------------

static bool in_rect(float mx, float my, int rx, int ry, int rw, int rh) {
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
}

static void start_edit(Inspector *ins, EditField field, const Agent *a) {
    ins->editField = field;
    float val = (field == EDIT_PERSONAL_VALUE) ? a->personalValue
                                               : a->expectedMarketValue;
    snprintf(ins->inputBuf, sizeof(ins->inputBuf), "%.2f", val);
    ins->inputLen = (int)strlen(ins->inputBuf);
}

static void apply_edit(Inspector *ins, Agent *agents) {
    if (ins->editField == EDIT_NONE || ins->selectedId < 0) return;
    float val = (float)atof(ins->inputBuf);
    if (val < 0.0f) val = 0.0f;
    if (ins->editField == EDIT_PERSONAL_VALUE)
        agents[ins->selectedId].personalValue = val;
    else
        agents[ins->selectedId].expectedMarketValue = val;
    ins->editField = EDIT_NONE;
}

// ---- public API ------------------------------------------------------------

void inspector_init(Inspector *ins) {
    ins->selectedId  = -1;
    ins->editField   = EDIT_NONE;
    ins->inputLen    = 0;
    ins->inputBuf[0] = '\0';
}

bool inspector_update(Inspector *ins, Agent *agents, int agentCount) {
    // Keyboard input while editing
    if (ins->editField != EDIT_NONE) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            bool validChar = (ch >= '0' && ch <= '9') || ch == '.' || ch == '-';
            if (validChar && ins->inputLen < 30) {
                ins->inputBuf[ins->inputLen++] = (char)ch;
                ins->inputBuf[ins->inputLen]   = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && ins->inputLen > 0)
            ins->inputBuf[--ins->inputLen] = '\0';
        if (IsKeyPressed(KEY_ENTER))
            apply_edit(ins, agents);
        if (IsKeyPressed(KEY_ESCAPE))
            ins->editField = EDIT_NONE;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();

    // Panel interactions (check before agent circles so clicks don't fall through)
    if (ins->selectedId >= 0 && in_rect(m.x, m.y, INS_X, INS_Y, INS_W, INS_H)) {
        // Close button
        if (in_rect(m.x, m.y, INS_X + INS_W - 24, INS_Y + 4, 20, 22)) {
            ins->selectedId = -1;
            ins->editField  = EDIT_NONE;
            return true;
        }
        // Editable rows
        if (in_rect(m.x, m.y, INS_X, ROW_Y_PV,  INS_W, ROW_H))
            start_edit(ins, EDIT_PERSONAL_VALUE, &agents[ins->selectedId]);
        else if (in_rect(m.x, m.y, INS_X, ROW_Y_EMV, INS_W, ROW_H))
            start_edit(ins, EDIT_EXPECTED_VALUE, &agents[ins->selectedId]);
        else
            ins->editField = EDIT_NONE;
        return true;
    }

    // Agent sprite hit-test (only in world area)
    if (m.y <= WORLD_AREA_H) {
        float spriteDisp = SPRITE_FRAME_SIZE * 2.0f;
        float agentY = (float)GROUND_Y - spriteDisp / 2.0f; // vertical center of sprite
        float hitR   = spriteDisp / 2.0f;
        for (int i = 0; i < agentCount; i++) {
            float dx = m.x - agents[i].x;
            float dy = m.y - agentY;
            if (dx*dx + dy*dy <= hitR * hitR) {
                if (ins->selectedId == i) {
                    ins->selectedId = -1;   // click same agent → deselect
                    ins->editField  = EDIT_NONE;
                } else {
                    ins->selectedId = i;
                    ins->editField  = EDIT_NONE;
                }
                return true;
            }
        }
        // Clicked world area, not on agent or panel → deselect
        ins->selectedId = -1;
        ins->editField  = EDIT_NONE;
    }

    return false;
}

// ---- rendering -------------------------------------------------------------

static void draw_row(int y, const char *label, const char *value,
                     bool editable, bool editing, Color valueColor) {
    Color bg = editing  ? (Color){ 20,  60,  90, 255}
             : editable ? (Color){ 35,  48,  60, 255}
                        : (Color){ 22,  30,  40, 255};
    DrawRectangle(INS_X, y, INS_W, ROW_H - 1, bg);
    if (editable && !editing)
        DrawRectangleLines(INS_X, y, INS_W, ROW_H - 1, (Color){70, 95, 115, 255});

    DrawText(label, INS_X + 8, y + 6, 13, (Color){170, 175, 185, 255});

    if (editing) {
        bool cursor = (int)(GetTime() * 2) % 2 == 0;
        char display[36];
        snprintf(display, sizeof(display), "%s%s", value, cursor ? "|" : " ");
        DrawText(display, INS_X + INS_W - MeasureText(display, 13) - 8, y + 6, 13, WHITE);
    } else {
        DrawText(value, INS_X + INS_W - MeasureText(value, 13) - 8, y + 6, 13, valueColor);
    }
}

void inspector_render(const Inspector *ins, const Agent *agents) {
    if (ins->selectedId < 0) return;
    const Agent *a = &agents[ins->selectedId];

    // Highlight ring around selected agent (centered on sprite)
    float spriteDisp = SPRITE_FRAME_SIZE * 2.0f; // matches SPRITE_DISP in render.c
    float cy = (float)GROUND_Y - spriteDisp / 2.0f;
    DrawCircleLines((int)a->x, (int)cy, (int)(spriteDisp / 2.0f) + 3, WHITE);

    // Panel shadow / background
    DrawRectangle(INS_X + 3, INS_Y + 3, INS_W, INS_H, (Color){0, 0, 0, 120});
    DrawRectangle(INS_X, INS_Y, INS_W, INS_H, (Color){15, 22, 32, 235});
    DrawRectangleLines(INS_X, INS_Y, INS_W, INS_H, (Color){90, 120, 150, 255});

    // Header bar
    DrawRectangle(INS_X, INS_Y, INS_W, HDR_H, (Color){28, 48, 78, 255});
    char title[32];
    snprintf(title, sizeof(title), "Agent #%d", a->id);
    DrawText(title, INS_X + 8, INS_Y + 7, 15, WHITE);
    DrawText("[X]", INS_X + INS_W - 30, INS_Y + 7, 14, (Color){200, 80, 80, 255});

    // ---- info rows ----
    char buf[48];

    snprintf(buf, sizeof(buf), "%d", a->id);
    draw_row(ROW_Y_ID, "Agent ID", buf, false, false, LIGHTGRAY);

    int isBuyer = (a->personalValue > a->expectedMarketValue);
    draw_row(ROW_Y_STATUS, "Status",
             isBuyer ? "Buyer" : "Seller", false, false,
             isBuyer ? (Color){60, 210, 90, 255} : (Color){220, 70, 70, 255});

    // Separator + hint
    DrawRectangle(INS_X, ROW_Y_PV - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});
    DrawText("click value to edit", INS_X + INS_W - 118, ROW_Y_PV - SEP_H + 1,
             10, (Color){90, 110, 130, 255});

    // ---- editable rows ----
    snprintf(buf, sizeof(buf), "%.2f", a->personalValue);
    draw_row(ROW_Y_PV, "Personal Value",
             ins->editField == EDIT_PERSONAL_VALUE ? ins->inputBuf : buf,
             true, ins->editField == EDIT_PERSONAL_VALUE,
             (Color){80, 140, 255, 255});

    snprintf(buf, sizeof(buf), "%.2f", a->expectedMarketValue);
    draw_row(ROW_Y_EMV, "Expected Mkt Val",
             ins->editField == EDIT_EXPECTED_VALUE ? ins->inputBuf : buf,
             true, ins->editField == EDIT_EXPECTED_VALUE,
             (Color){60, 210, 90, 255});

    // Separator
    DrawRectangle(INS_X, ROW_Y_TIMER - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});

    // ---- non-editable info ----
    snprintf(buf, sizeof(buf), "%.1f / %.1fs", a->timeSinceLastTrade,
             a->maxTimeSinceLastTrade);
    draw_row(ROW_Y_TIMER, "Idle Timer", buf, false, false,
             (Color){210, 185, 90, 255});

    if (a->targetType == TARGET_AGENT)
        snprintf(buf, sizeof(buf), "Agent #%d", a->targetId);
    else
        snprintf(buf, sizeof(buf), "Pos %.0fpx", a->targetX);
    draw_row(ROW_Y_TARGET, "Target", buf, false, false, LIGHTGRAY);
}
