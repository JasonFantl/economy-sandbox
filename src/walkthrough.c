#include "walkthrough.h"
#include "econ.h"
#include "market.h"
#include "agent.h"
#include "render.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Step render functions
// ---------------------------------------------------------------------------

static void render_c1s1p1(const SimContext *ctx, int x, int y, int w, int h) {
    panel_price_history(ctx->avh, ctx->pvh, ctx->agents, ctx->count,
                        MARKET_WOOD, x, y, w, h, 0.0f, false);
}

static void render_c1s2p1(const SimContext *ctx, int x, int y, int w, int h) {
    panel_price_history(ctx->avh, ctx->pvh, ctx->agents, ctx->count,
                        MARKET_WOOD, x, y, w, h, 0.0f, false);
}

static void render_c1s2p2(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    panel_price_history(ctx->avh, ctx->pvh, ctx->agents, ctx->count,
                        MARKET_WOOD, x, y, hw, h, 0.0f, false);
    panel_supply_demand(ctx->agents, ctx->count, MARKET_WOOD,
                        x + hw + gap, y, hw, h);
}

static void render_c1s2p3(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    panel_price_history(ctx->avh, ctx->pvh, ctx->agents, ctx->count,
                        MARKET_WOOD, x, y, hw, h, 0.0f, false);
    panel_wealth(ctx->agents, ctx->count, MARKET_WOOD,
                 x + hw + gap, y, hw, h);
}

static float avg_eq(const SimContext *ctx, int mid) {
    float sum = 0.0f;
    for (int i = 0; i < ctx->count; i++)
        sum += ctx->agents[i].econ.markets[mid].maxUtility;
    return ctx->count > 0 ? sum / (float)ctx->count : 0.0f;
}

static void render_c1s2p4(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2, hh = (h - gap) / 2;
    float eq = avg_eq(ctx, MARKET_WOOD);
    panel_price_history  (ctx->avh, ctx->pvh, ctx->agents, ctx->count,
                          MARKET_WOOD, x,         y,         hw, hh, eq, true);
    panel_supply_demand  (ctx->agents, ctx->count, MARKET_WOOD,
                          x + hw + gap, y,         hw, hh);
    panel_valuation_dist (ctx->agents, ctx->count, MARKET_WOOD,
                          x,            y + hh + gap, hw, hh, eq);
    panel_wealth         (ctx->agents, ctx->count, MARKET_WOOD,
                          x + hw + gap, y + hh + gap, hw, hh);
}

static void render_c1s2p5(const SimContext *ctx, int x, int y, int w, int h) {
    render_c1s2p4(ctx, x, y, w, h);
    panel_ctrl_decay_render(ctx->decay, 0, 0);
}

static void render_c1s2p6(const SimContext *ctx, int x, int y, int w, int h) {
    render_c1s2p4(ctx, x, y, w, h);
    panel_ctrl_decay_render(ctx->decay, 0, 0);
}

static void render_c1s2p7(const SimContext *ctx, int x, int y, int w, int h) {
    render_c1s2p4(ctx, x, y, w, h);
    panel_ctrl_influence_render(ctx->inf, 0, 0);
}

// ---------------------------------------------------------------------------
// Chapter / Scene / Step data
// ---------------------------------------------------------------------------

static const ChapterDef CHAPTERS[1] = {
{
    "Chapter 1: One Good", 2,
    {
        // Scene 1: Two Agents
        {
            "Scene 1: Two Agents", 1,
            {
                {
                    2, false, true, false, false, false, false, 20.0f, 60.0f,
                    "Two Agents",
                    "Two agents have different ideas of what a unit of wood is worth. "
                    "Each time they fail to trade, they nudge their expected price "
                    "toward their own valuation. Watch the prices converge over time.",
                    render_c1s1p1
                }
            }
        },
        // Scene 2: Many Agents
        {
            "Scene 2: Many Agents", 7,
            {
                {
                    100, false, true, false, false, false, false, 20.0f, 60.0f,
                    "Many Agents",
                    "With 100 agents, each with a different fixed valuation, all price "
                    "expectations converge toward a shared market price through repeated "
                    "gossip and failed trade attempts.",
                    render_c1s2p1
                },
                {
                    100, false, true, false, false, false, false, 20.0f, 60.0f,
                    "Supply & Demand",
                    "The supply curve shows agents ranked by their minimum sell price. "
                    "The demand curve shows agents ranked by their maximum buy price. "
                    "They cross at the equilibrium price - the natural price the market converges to.",
                    render_c1s2p2
                },
                {
                    100, false, false, false, false, false, false, 20.0f, 60.0f,
                    "Market Collapse",
                    "Without debt, buyers who run out of money can no longer trade. "
                    "As money piles up in fewer hands, the market seizes. "
                    "Watch the wealth distribution become extreme.",
                    render_c1s2p3
                },
                {
                    100, true, false, false, false, false, false, 20.0f, 60.0f,
                    "Diminishing Returns",
                    "Agents now have diminishing marginal utility - each additional unit "
                    "of wood is worth less than the last. This creates a natural stopping "
                    "point: agents stop buying when the price exceeds their marginal value. "
                    "The market stabilises.",
                    render_c1s2p4
                },
                {
                    100, true, false, false, true, false, false, 20.0f, 60.0f,
                    "Goods Decay",
                    "Wood now rots over time. As inventory evaporates, the supply curve "
                    "shrinks and prices rise. Try adjusting the decay rate to see how "
                    "the market responds.",
                    render_c1s2p5
                },
                {
                    100, true, false, true, true, false, false, 20.0f, 60.0f,
                    "Production",
                    "Agents can now chop wood. When the market price exceeds their cost "
                    "of effort, they produce and sell. Adjust the chop yield to flood "
                    "or starve the market.",
                    render_c1s2p6
                },
                {
                    100, true, false, true, true, true, false, 20.0f, 60.0f,
                    "Leisure",
                    "Agents now value leisure - resting has diminishing utility too. "
                    "They balance chopping, trading, and resting. "
                    "Raise or lower the leisure value to see how it shifts supply.",
                    render_c1s2p7
                }
            }
        }
    }
}
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static const StepDef *current_step(const WalkthroughState *wt) {
    return &CHAPTERS[wt->chapter].scenes[wt->scene].steps[wt->step];
}

const StepDef *walkthrough_current_step(const WalkthroughState *wt) {
    return current_step(wt);
}

// ---------------------------------------------------------------------------
// Apply step config
// ---------------------------------------------------------------------------

void walkthrough_apply(WalkthroughState *wt, SimContext *ctx) {
    const StepDef *s = current_step(wt);
    g_diminishing_returns = s->diminishingReturns;
    g_production_enabled  = s->productionEnabled;
    g_leisure_enabled     = s->leisureEnabled;
    g_two_goods           = s->twoGoods;
    g_allow_debt          = s->debtAllowed ? 1 : 0;
    if (!s->decayEnabled) {
        g_wood_decay_rate  = 0.0f;
        g_chair_decay_rate = 0.0f;
    } else {
        if (g_wood_decay_rate == 0.0f)  g_wood_decay_rate  = 0.003f;
        if (g_chair_decay_rate == 0.0f) g_chair_decay_rate = 0.003f;
    }
    // Re-init agents with step's agent count
    ctx->count = s->numAgents;
    agents_init(ctx->agents, ctx->count, ctx->worldW, ctx->worldH);
    // Randomise maxUtility into the step's baseValue range for wood
    for (int i = 0; i < ctx->count; i++) {
        float t = (float)GetRandomValue(0, 10000) / 10000.0f;
        ctx->agents[i].econ.markets[MARKET_WOOD].maxUtility =
            s->baseValueMin + t * (s->baseValueMax - s->baseValueMin);
        ctx->agents[i].econ.markets[MARKET_WOOD].priceExpectation =
            ctx->agents[i].econ.markets[MARKET_WOOD].maxUtility;
    }
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void walkthrough_init(WalkthroughState *wt, SimContext *ctx) {
    wt->chapter = 0; wt->scene = 0; wt->step = 0; wt->active = true;
    walkthrough_apply(wt, ctx);
}

bool walkthrough_next_step(WalkthroughState *wt, SimContext *ctx) {
    const SceneDef *sc = &CHAPTERS[wt->chapter].scenes[wt->scene];
    if (wt->step + 1 < sc->stepCount) {
        wt->step++;
    } else {
        const ChapterDef *ch = &CHAPTERS[wt->chapter];
        if (wt->scene + 1 < ch->sceneCount) {
            wt->scene++; wt->step = 0;
        } else {
            walkthrough_exit(wt);
            return false;
        }
    }
    walkthrough_apply(wt, ctx);
    return true;
}

bool walkthrough_prev_step(WalkthroughState *wt, SimContext *ctx) {
    if (wt->step > 0) {
        wt->step--;
    } else if (wt->scene > 0) {
        wt->scene--;
        wt->step = CHAPTERS[wt->chapter].scenes[wt->scene].stepCount - 1;
    } else {
        return false;
    }
    walkthrough_apply(wt, ctx);
    return true;
}

void walkthrough_restart(WalkthroughState *wt, SimContext *ctx) {
    walkthrough_apply(wt, ctx);
}

void walkthrough_exit(WalkthroughState *wt) {
    wt->active = false;
    g_diminishing_returns = true;
    g_production_enabled  = true;
    g_leisure_enabled     = true;
    g_two_goods           = true;
    g_allow_debt          = 0;
    g_wood_decay_rate     = 0.000f;
    g_chair_decay_rate    = 0.003f;
}

// ---------------------------------------------------------------------------
// Input handler
// ---------------------------------------------------------------------------

bool walkthrough_handle_input(WalkthroughState *wt, SimContext *ctx) {
    if (!wt->active) return false;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_N)) {
        walkthrough_next_step(wt, ctx); return true;
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_P)) {
        walkthrough_prev_step(wt, ctx); return true;
    }
    if (IsKeyPressed(KEY_R)) {
        walkthrough_restart(wt, ctx); return true;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        walkthrough_exit(wt); return true;
    }
    Vector2 m = GetMousePosition();
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    // Prev button: x 10..80, y 2..32
    if (m.x >= 10 && m.x <= 80 && m.y >= 2 && m.y <= 32) {
        walkthrough_prev_step(wt, ctx); return true;
    }
    // Next button
    if (m.x >= SCREEN_W-80 && m.x <= SCREEN_W-10 && m.y >= 2 && m.y <= 32) {
        walkthrough_next_step(wt, ctx); return true;
    }
    // Restart button
    if (m.x >= SCREEN_W-170 && m.x <= SCREEN_W-90 && m.y >= 2 && m.y <= 32) {
        walkthrough_restart(wt, ctx); return true;
    }
    // Exit button
    if (m.x >= SCREEN_W-260 && m.x <= SCREEN_W-180 && m.y >= 2 && m.y <= 32) {
        walkthrough_exit(wt); return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Word-wrap helper
// ---------------------------------------------------------------------------

static void draw_wrapped_text(const char *text, int x, int y, int maxW,
                               int fontSize, Color col) {
    char line[256];
    int lineStart = 0, lineLen = 0, lastSpace = -1;
    int ly = y;
    for (int i = 0; ; i++) {
        char c = text[i];
        if (c == '\0' || c == '\n') {
            int copyLen = lineLen < 255 ? lineLen : 255;
            strncpy(line, text + lineStart, (size_t)copyLen);
            line[copyLen] = '\0';
            DrawText(line, x, ly, fontSize, col);
            ly += fontSize + 4;
            if (c == '\0') break;
            lineStart = i + 1; lineLen = 0; lastSpace = -1;
        } else {
            if (c == ' ') lastSpace = i;
            lineLen++;
            int copyLen = lineLen < 255 ? lineLen : 255;
            strncpy(line, text + lineStart, (size_t)copyLen);
            line[copyLen] = '\0';
            if (MeasureText(line, fontSize) > maxW && lastSpace >= 0) {
                int cutLen = lastSpace - lineStart;
                if (cutLen > 255) cutLen = 255;
                strncpy(line, text + lineStart, (size_t)cutLen);
                line[cutLen] = '\0';
                DrawText(line, x, ly, fontSize, col);
                ly += fontSize + 4;
                lineStart = lastSpace + 1;
                lineLen = i - lineStart + 1;
                lastSpace = -1;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Overlay renderer
// ---------------------------------------------------------------------------

void walkthrough_render_overlay(const WalkthroughState *wt) {
    if (!wt->active) return;
    const StepDef   *s  = current_step(wt);
    const SceneDef  *sc = &CHAPTERS[wt->chapter].scenes[wt->scene];
    const ChapterDef*ch = &CHAPTERS[wt->chapter];

    // Nav bar background
    DrawRectangle(0, 0, SCREEN_W, WTHROUGH_NAV_H, (Color){18, 26, 40, 240});
    DrawLine(0, WTHROUGH_NAV_H, SCREEN_W, WTHROUGH_NAV_H, (Color){70, 90, 120, 255});

    Vector2 mouse = GetMousePosition();

    // Prev button
    bool hPrev = mouse.x>=10&&mouse.x<=80&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(10, 4, 70, WTHROUGH_NAV_H-8, hPrev?(Color){60,90,140,255}:(Color){35,55,90,255});
    DrawRectangleLines(10, 4, 70, WTHROUGH_NAV_H-8, (Color){80,120,180,200});
    DrawText("< Prev", 14, 10, 13, hPrev ? WHITE : (Color){180,190,210,255});

    // Next button
    bool hNext = mouse.x>=SCREEN_W-80&&mouse.x<=SCREEN_W-10&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(SCREEN_W-80, 4, 70, WTHROUGH_NAV_H-8, hNext?(Color){60,90,140,255}:(Color){35,55,90,255});
    DrawRectangleLines(SCREEN_W-80, 4, 70, WTHROUGH_NAV_H-8, (Color){80,120,180,200});
    DrawText("Next >", SCREEN_W-76, 10, 13, hNext ? WHITE : (Color){180,190,210,255});

    // Restart button
    bool hRes = mouse.x>=SCREEN_W-170&&mouse.x<=SCREEN_W-90&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(SCREEN_W-170, 4, 70, WTHROUGH_NAV_H-8, hRes?(Color){80,50,20,255}:(Color){50,32,12,255});
    DrawRectangleLines(SCREEN_W-170, 4, 70, WTHROUGH_NAV_H-8, (Color){160,100,40,200});
    DrawText("Restart", SCREEN_W-166, 10, 13, hRes ? WHITE : (Color){180,140,80,255});

    // Exit button
    bool hExit = mouse.x>=SCREEN_W-260&&mouse.x<=SCREEN_W-180&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(SCREEN_W-260, 4, 70, WTHROUGH_NAV_H-8, hExit?(Color){80,20,20,255}:(Color){50,12,12,255});
    DrawRectangleLines(SCREEN_W-260, 4, 70, WTHROUGH_NAV_H-8, (Color){160,60,60,200});
    DrawText("Exit", SCREEN_W-248, 10, 13, hExit ? WHITE : (Color){200,100,100,255});

    // Centre label
    char label[128];
    snprintf(label, sizeof(label), "%s  -  %s  -  Step %d/%d",
             ch->title, sc->title, wt->step + 1, sc->stepCount);
    int lw = MeasureText(label, 12);
    DrawText(label, (SCREEN_W - lw) / 2, 6, 12, (Color){200,210,230,255});

    // Step title below centre label
    int stw = MeasureText(s->title, 11);
    DrawText(s->title, (SCREEN_W - stw) / 2, 21, 11, (Color){120,160,220,200});

    // Text box at bottom of world viewport
    int textY = WTHROUGH_NAV_H + WORLD_VIEW_H - 75;
    DrawRectangle(0, textY, SCREEN_W, 75, (Color){0, 0, 0, 160});
    DrawLine(0, textY, SCREEN_W, textY, (Color){70, 90, 120, 200});

    draw_wrapped_text(s->text, 20, textY + 8, SCREEN_W - 40, 13,
                      (Color){200, 210, 230, 240});
}
