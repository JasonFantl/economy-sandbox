#include "controls.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Panel geometry (top-left of world area, below the legend)
#define PNL_X   10
#define PNL_Y   32
#define PNL_W  210
#define HDR_H   22
#define ROW_H   24
#define SEP      4
#define BTN_H   26
#define PAD      5
#define PNL_H  (HDR_H + SEP + ROW_H + ROW_H + SEP + BTN_H + PAD)

#define ROW_Y_N   (PNL_Y + HDR_H + SEP)
#define ROW_Y_F   (ROW_Y_N + ROW_H)
#define BTN_Y     (ROW_Y_F + ROW_H + SEP)

static bool in_rect(float mx, float my, int rx, int ry, int rw, int rh) {
    return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
}

// ---- helpers ---------------------------------------------------------------

static void start_edit(InfluencePanel *p, InfEditField field) {
    p->editing = field;
    if (field == INF_EDIT_N)
        snprintf(p->buf, sizeof(p->buf), "%d", p->n);
    else
        snprintf(p->buf, sizeof(p->buf), "%.1f", p->f);
    p->bufLen = (int)strlen(p->buf);
}

static void apply_edit(InfluencePanel *p) {
    if (p->editing == INF_EDIT_N) {
        int v = atoi(p->buf);
        if (v > 0) p->n = v;
    } else if (p->editing == INF_EDIT_F) {
        p->f = (float)atof(p->buf);
    }
    p->editing = INF_EDIT_NONE;
}

// ---- public API ------------------------------------------------------------

void influence_panel_init(InfluencePanel *p) {
    p->n       = 30;
    p->f       = 5.0f;
    p->editing = INF_EDIT_NONE;
    p->bufLen  = 0;
    p->buf[0]  = '\0';
}

bool influence_panel_update(InfluencePanel *p, Agent *agents, int agentCount) {
    // Keyboard input while editing
    if (p->editing != INF_EDIT_NONE) {
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            // Allow digits, dot, and minus (for negative delta)
            bool ok = (ch >= '0' && ch <= '9') || ch == '.'
                    || (ch == '-' && p->bufLen == 0);
            if (ok && p->bufLen < 14) {
                p->buf[p->bufLen++] = (char)ch;
                p->buf[p->bufLen]   = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && p->bufLen > 0)
            p->buf[--p->bufLen] = '\0';
        if (IsKeyPressed(KEY_ENTER))  apply_edit(p);
        if (IsKeyPressed(KEY_ESCAPE)) p->editing = INF_EDIT_NONE;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();
    if (!in_rect(m.x, m.y, PNL_X, PNL_Y, PNL_W, PNL_H)) return false;

    // N row
    if (in_rect(m.x, m.y, PNL_X, ROW_Y_N, PNL_W, ROW_H)) {
        if (p->editing == INF_EDIT_N) apply_edit(p);
        else start_edit(p, INF_EDIT_N);
        return true;
    }
    // F row
    if (in_rect(m.x, m.y, PNL_X, ROW_Y_F, PNL_W, ROW_H)) {
        if (p->editing == INF_EDIT_F) apply_edit(p);
        else start_edit(p, INF_EDIT_F);
        return true;
    }
    // Button
    if (in_rect(m.x, m.y, PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H)) {
        if (p->editing != INF_EDIT_NONE) apply_edit(p); // commit pending edit first
        agents_influence(agents, agentCount, p->n, p->f);
        return true;
    }

    return true; // consumed (clicked panel but not a specific widget)
}

void influence_panel_render(const InfluencePanel *p) {
    // Shadow + background
    DrawRectangle(PNL_X + 2, PNL_Y + 2, PNL_W, PNL_H, (Color){0, 0, 0, 100});
    DrawRectangle(PNL_X, PNL_Y, PNL_W, PNL_H, (Color){15, 22, 32, 220});
    DrawRectangleLines(PNL_X, PNL_Y, PNL_W, PNL_H, (Color){80, 110, 140, 255});

    // Header
    DrawRectangle(PNL_X, PNL_Y, PNL_W, HDR_H, (Color){28, 48, 78, 255});
    DrawText("Influence Market", PNL_X + 6, PNL_Y + 5, 13, WHITE);

    // Helper: draw one editable row
    // N row
    {
        bool editing = (p->editing == INF_EDIT_N);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(PNL_X, ROW_Y_N, PNL_W, ROW_H - 1, bg);
        DrawRectangleLines(PNL_X, ROW_Y_N, PNL_W, ROW_H - 1,
                           (Color){70, 95, 115, 255});
        DrawText("N agents:", PNL_X + 6, ROW_Y_N + 5, 13,
                 (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6,
                     ROW_Y_N + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%d", p->n);
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6,
                     ROW_Y_N + 5, 13, (Color){80, 180, 255, 255});
        }
    }

    // F row
    {
        bool editing = (p->editing == INF_EDIT_F);
        Color bg = editing ? (Color){20, 60, 90, 255} : (Color){35, 48, 60, 255};
        DrawRectangle(PNL_X, ROW_Y_F, PNL_W, ROW_H - 1, bg);
        DrawRectangleLines(PNL_X, ROW_Y_F, PNL_W, ROW_H - 1,
                           (Color){70, 95, 115, 255});
        DrawText("\xce\x94 value:", PNL_X + 6, ROW_Y_F + 5, 13,
                 (Color){170, 175, 185, 255});
        char val[24];
        if (editing) {
            bool cur = (int)(GetTime() * 2) % 2 == 0;
            snprintf(val, sizeof(val), "%s%s", p->buf, cur ? "|" : " ");
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6,
                     ROW_Y_F + 5, 13, WHITE);
        } else {
            snprintf(val, sizeof(val), "%+.1f", p->f);
            Color col = (p->f >= 0) ? (Color){60, 210, 90, 255}
                                    : (Color){220, 70, 70, 255};
            DrawText(val, PNL_X + PNL_W - MeasureText(val, 13) - 6,
                     ROW_Y_F + 5, 13, col);
        }
    }

    // Apply button
    Vector2 mouse = GetMousePosition();
    bool hover = in_rect(mouse.x, mouse.y, PNL_X + PAD, BTN_Y,
                         PNL_W - 2*PAD, BTN_H);
    Color btnBg  = hover ? (Color){60, 100, 160, 255} : (Color){40, 70, 120, 255};
    Color btnBdr = hover ? (Color){120, 170, 230, 255} : (Color){80, 120, 180, 255};
    DrawRectangle(PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H, btnBg);
    DrawRectangleLines(PNL_X + PAD, BTN_Y, PNL_W - 2*PAD, BTN_H, btnBdr);
    const char *btnLabel = "Apply Influence";
    DrawText(btnLabel,
             PNL_X + PAD + (PNL_W - 2*PAD - MeasureText(btnLabel, 13)) / 2,
             BTN_Y + 6, 13, WHITE);
}
