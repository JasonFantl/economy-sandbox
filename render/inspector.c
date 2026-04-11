#include "render/inspector.h"
#include "render/render.h"
#include "world/tileset.h"
#include "raygui.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>

// Panel geometry — X is computed at render time so it pins to the right edge
#define INS_Y     30
#define INS_W    330
static inline int ins_x(void) { return GetScreenWidth() - INS_W - 30; }
#define HDR_H     24    // GuiWindowBox status bar height
#define ROW_H     22
#define MKT_HDR_H 14
#define SEP_H      4
#define PAD        6

// Row Y positions (content starts below the 24px window title bar)
#define ROW_Y_FIRST      (INS_Y + HDR_H + PAD)
#define ROW_Y_ID         ROW_Y_FIRST
#define ROW_Y_ACTION     (ROW_Y_ID      + ROW_H)
#define ROW_Y_WOOD_HDR   (ROW_Y_ACTION  + ROW_H + SEP_H)
#define ROW_Y_WOOD_GOODS (ROW_Y_WOOD_HDR   + MKT_HDR_H)
#define ROW_Y_WOOD_S     (ROW_Y_WOOD_GOODS + ROW_H)
#define ROW_Y_WOOD_EMV   (ROW_Y_WOOD_S     + ROW_H)
#define ROW_Y_WOOD_PRICES (ROW_Y_WOOD_EMV  + ROW_H)
#define ROW_Y_CHAIR_HDR  (ROW_Y_WOOD_PRICES + ROW_H + SEP_H)
#define ROW_Y_CHAIR_GOODS (ROW_Y_CHAIR_HDR  + MKT_HDR_H)
#define ROW_Y_CHAIR_S    (ROW_Y_CHAIR_GOODS + ROW_H)
#define ROW_Y_CHAIR_EMV  (ROW_Y_CHAIR_S     + ROW_H)
#define ROW_Y_CHAIR_PRICES (ROW_Y_CHAIR_EMV + ROW_H)
#define ROW_Y_MONEY      (ROW_Y_CHAIR_PRICES + ROW_H + SEP_H)
#define ROW_Y_LEISURE    (ROW_Y_MONEY   + ROW_H)
#define ROW_Y_TARGET     (ROW_Y_LEISURE + ROW_H)

#define INS_H (HDR_H + 2*PAD + 2*ROW_H + SEP_H + MKT_HDR_H + 4*ROW_H + SEP_H + MKT_HDR_H + 4*ROW_H + SEP_H + 3*ROW_H)

// Label column width for editable rows; value box takes the right portion
#define EDIT_LBL_W  120
#define EDIT_BOX_X  (ins_x() + PAD + EDIT_LBL_W + 2)
#define EDIT_BOX_W  (INS_W - PAD - EDIT_LBL_W - 2 - PAD/2)

// ---------------------------------------------------------------------------
// Non-editable row helpers (unchanged visual style)
// ---------------------------------------------------------------------------

static void draw_row(int y, const char *label, const char *value, Color valueColor) {
    DrawRectangle(ins_x(), y, INS_W, ROW_H - 1, (Color){22, 30, 40, 255});
    DrawTextF(label, ins_x() + 8, y + 5, 12, (Color){170, 175, 185, 255});
    DrawTextF(value, ins_x() + INS_W - MeasureTextF(value, 12) - 8, y + 5, 12, valueColor);
}

static void draw_split_row(int y, const char *labelL, const char *valL, Color colL,
                                   const char *labelR, const char *valR, Color colR) {
    DrawRectangle(ins_x(), y, INS_W, ROW_H - 1, (Color){22, 30, 40, 255});
    int mid = ins_x() + INS_W / 2;
    DrawTextF(labelL, ins_x() + 8,  y + 5, 12, (Color){170, 175, 185, 255});
    DrawTextF(valL,   mid - MeasureTextF(valL, 12) - 6, y + 5, 12, colL);
    DrawTextF(labelR, mid + 6,    y + 5, 12, (Color){170, 175, 185, 255});
    DrawTextF(valR,   ins_x() + INS_W - MeasureTextF(valR, 12) - 8, y + 5, 12, colR);
}

static void draw_market_section_header(int y, const char *title, Color col) {
    DrawRectangle(ins_x(), y, INS_W, MKT_HDR_H, (Color){28, 38, 55, 255});
    DrawRectangleLines(ins_x(), y, INS_W, MKT_HDR_H, (Color){55, 72, 100, 200});
    DrawTextF(title, ins_x() + 8, y + 2, 11, col);
}

// ---------------------------------------------------------------------------
// Init / update
// ---------------------------------------------------------------------------

static void reset_edit_state(Inspector *ins) {
    ins->editWoodUtil = ins->editWoodPrice = false;
    ins->editChairUtil = ins->editChairPrice = false;
}

void inspector_init(Inspector *ins) {
    ins->selectedId = -1;
    reset_edit_state(ins);
}

bool inspector_update(Inspector *ins, Agent *agents, int agentCount,
                      float camX, float camY, float camZoom) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m = GetMousePosition();

    // Clicks inside the panel are consumed here to block world interaction;
    // GuiWindowBox close and field editing are handled in inspector_render.
    if (ins->selectedId >= 0 &&
        m.x >= ins_x() && m.x <= ins_x() + INS_W &&
        m.y >= INS_Y && m.y <= INS_Y + INS_H)
        return true;

    // World-space agent sprite hit-test
    if (m.y >= g_world_view_y && m.y <= g_world_view_y + WORLD_VIEW_H) {
        float wx = (m.x - (float)GetScreenWidth() * 0.5f) / camZoom + camX;
        float wy = (m.y - ((float)g_world_view_y + (float)WORLD_VIEW_H * 0.5f)) / camZoom + camY;
        float hitR = (float)AGENT_DISP * 0.5f;
        for (int i = 0; i < agentCount; i++) {
            float dx = wx - agents[i].body.x;
            float dy = wy - agents[i].body.y;
            if (dx*dx + dy*dy <= hitR * hitR) {
                if (ins->selectedId == i) {
                    ins->selectedId = -1;
                } else {
                    ins->selectedId = i;
                    reset_edit_state(ins);
                    // Pre-populate float buffers for the new agent
                    snprintf(ins->bufWoodUtil,   sizeof(ins->bufWoodUtil),   "%.2f",
                             agents[i].econ.markets[MARKET_WOOD].maxUtility);
                    snprintf(ins->bufWoodPrice,  sizeof(ins->bufWoodPrice),  "%.2f",
                             agents[i].econ.markets[MARKET_WOOD].priceExpectation);
                    snprintf(ins->bufChairUtil,  sizeof(ins->bufChairUtil),  "%.2f",
                             agents[i].econ.markets[MARKET_CHAIR].maxUtility);
                    snprintf(ins->bufChairPrice, sizeof(ins->bufChairPrice), "%.2f",
                             agents[i].econ.markets[MARKET_CHAIR].priceExpectation);
                }
                return true;
            }
        }
        ins->selectedId = -1;
        reset_edit_state(ins);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void inspector_render(Inspector *ins, Agent *agents,
                      float camX, float camY, float camZoom) {
    if (ins->selectedId < 0) return;
    Agent *a = &agents[ins->selectedId];

    // Selection highlight ring
    float sx = (a->body.x - camX) * camZoom + (float)GetScreenWidth() * 0.5f;
    float sy = (a->body.y - camY) * camZoom + (float)g_world_view_y
                                             + (float)WORLD_VIEW_H * 0.5f;
    DrawCircleLines((int)sx, (int)sy, (float)(AGENT_DISP / 2 + 3) * camZoom, WHITE);

    // Window box (title bar + close button)
    char title[32];
    snprintf(title, sizeof(title), "Agent #%d", a->body.id);
    if (GuiWindowBox((Rectangle){ins_x(), INS_Y, INS_W, INS_H}, title)) {
        ins->selectedId = -1;
        reset_edit_state(ins);
        return;
    }

    char buf[48];

    // Static info rows
    snprintf(buf, sizeof(buf), "%d", a->body.id);
    draw_row(ROW_Y_ID, "Agent ID", buf, LIGHTGRAY);

    const char *actName;
    Color actCol;
    switch (a->econ.lastAction) {
        case ACTION_CHOP:  actName = "Chopping"; actCol = (Color){160, 100,  40, 255}; break;
        case ACTION_BUILD: actName = "Building"; actCol = (Color){220, 140,  60, 255}; break;
        default:           actName = "Leisure";  actCol = (Color){150, 150, 150, 255}; break;
    }
    draw_row(ROW_Y_ACTION, "Last Action", actName, actCol);

    DrawRectangle(ins_x(), ROW_Y_WOOD_HDR - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});
    DrawTextF("click to edit", ins_x() + INS_W - 90, ROW_Y_WOOD_HDR - SEP_H + 1,
             10, (Color){90, 110, 130, 255});

    // ---- Wood market ----
    draw_market_section_header(ROW_Y_WOOD_HDR, "WOOD MARKET", (Color){160, 100, 40, 255});
    const AgentMarket *mw = &a->econ.markets[MARKET_WOOD];
    snprintf(buf, sizeof(buf), "%d", mw->goods);
    draw_row(ROW_Y_WOOD_GOODS, "Goods", buf, (Color){200, 160, 80, 255});

    if (GuiValueBoxFloat((Rectangle){EDIT_BOX_X, ROW_Y_WOOD_S, EDIT_BOX_W, ROW_H-2},
                         "Max Utility", ins->bufWoodUtil,
                         &a->econ.markets[MARKET_WOOD].maxUtility, ins->editWoodUtil))
        ins->editWoodUtil = !ins->editWoodUtil;

    if (GuiValueBoxFloat((Rectangle){EDIT_BOX_X, ROW_Y_WOOD_EMV, EDIT_BOX_W, ROW_H-2},
                         "Price Expect.", ins->bufWoodPrice,
                         &a->econ.markets[MARKET_WOOD].priceExpectation, ins->editWoodPrice))
        ins->editWoodPrice = !ins->editWoodPrice;

    char buyBuf[24], sellBuf[24];
    snprintf(buyBuf,  sizeof(buyBuf),  "%.2f", marginal_buy_utility(mw));
    snprintf(sellBuf, sizeof(sellBuf), "%.2f", marginal_sell_utility(mw));
    draw_split_row(ROW_Y_WOOD_PRICES,
                   "Buy util:", buyBuf,  (Color){ 80, 200, 240, 255},
                   "Sell util:", sellBuf, (Color){240, 160,  80, 255});

    // ---- Chair market ----
    DrawRectangle(ins_x(), ROW_Y_CHAIR_HDR - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});
    draw_market_section_header(ROW_Y_CHAIR_HDR, "CHAIR MARKET", (Color){200, 140, 60, 255});
    const AgentMarket *mc = &a->econ.markets[MARKET_CHAIR];
    snprintf(buf, sizeof(buf), "%d", mc->goods);
    draw_row(ROW_Y_CHAIR_GOODS, "Goods", buf, (Color){200, 160, 80, 255});

    if (GuiValueBoxFloat((Rectangle){EDIT_BOX_X, ROW_Y_CHAIR_S, EDIT_BOX_W, ROW_H-2},
                         "Max Utility", ins->bufChairUtil,
                         &a->econ.markets[MARKET_CHAIR].maxUtility, ins->editChairUtil))
        ins->editChairUtil = !ins->editChairUtil;

    if (GuiValueBoxFloat((Rectangle){EDIT_BOX_X, ROW_Y_CHAIR_EMV, EDIT_BOX_W, ROW_H-2},
                         "Price Expect.", ins->bufChairPrice,
                         &a->econ.markets[MARKET_CHAIR].priceExpectation, ins->editChairPrice))
        ins->editChairPrice = !ins->editChairPrice;

    snprintf(buyBuf,  sizeof(buyBuf),  "%.2f", marginal_buy_utility(mc));
    snprintf(sellBuf, sizeof(sellBuf), "%.2f", marginal_sell_utility(mc));
    draw_split_row(ROW_Y_CHAIR_PRICES,
                   "Buy util:", buyBuf,  (Color){ 80, 200, 240, 255},
                   "Sell util:", sellBuf, (Color){240, 160,  80, 255});

    // ---- Shared stats ----
    DrawRectangle(ins_x(), ROW_Y_MONEY - SEP_H, INS_W, SEP_H, (Color){40, 60, 80, 255});
    snprintf(buf, sizeof(buf), "%.2f", a->econ.money);
    draw_row(ROW_Y_MONEY,   "Money",           buf, (Color){255, 215, 0, 255});
    snprintf(buf, sizeof(buf), "%.2f", a->econ.leisureUtility);
    draw_row(ROW_Y_LEISURE, "Leisure Utility", buf, (Color){150, 150, 150, 255});
    if (a->body.targetType == TARGET_AGENT)
        snprintf(buf, sizeof(buf), "Agent #%d", a->body.targetId);
    else
        snprintf(buf, sizeof(buf), "Pos %.0fpx", a->body.targetX);
    draw_row(ROW_Y_TARGET,  "Target",          buf, LIGHTGRAY);
}
