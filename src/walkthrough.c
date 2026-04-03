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
    int gap = 8, hw = (w - gap) / 2;
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         x, y, hw, h, 0.0f);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, x + hw + gap, y, hw, h, 0.0f, false);
}

static void render_c1s2p1(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         x, y, hw, h, 0.0f);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, x + hw + gap, y, hw, h, 0.0f, false);
}

static void render_c1s2p2(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         x, y, hw, h, 0.0f);
    panel_supply_demand(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                        x + hw + gap, y, hw, h);
}

static void render_c1s2p3(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2, hh = (h - gap) / 2;
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         x, y, hw, hh, 0.0f);
    panel_wealth(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                 x, y + hh + gap, hw, hh);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, x + hw + gap, y, hw, h, 0.0f, false);
}

static float avg_eq(const SimContext *ctx, int mid) {
    float sum = 0.0f;
    for (int i = 0; i < ctx->sim->count; i++)
        sum += ctx->sim->agents[i].econ.markets[mid].maxUtility;
    return ctx->sim->count > 0 ? sum / (float)ctx->sim->count : 0.0f;
}

static void render_c1s2p4(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2, hh = (h - gap) / 2;
    float eq = avg_eq(ctx, MARKET_WOOD);
    panel_price_history  (ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                          MARKET_WOOD, x,         y,         hw, hh, eq, true);
    panel_supply_demand  (ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                          x + hw + gap, y,         hw, hh);
    panel_valuation_dist (ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                          x,            y + hh + gap, hw, hh, eq);
    panel_wealth         (ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
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
// Step init functions
// ---------------------------------------------------------------------------

static void step_c1s1p1(void) {
    g_diminishing_returns = false;
    g_production_enabled  = false;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 1;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
}

static void step_c1s2p1(void) {
    g_diminishing_returns = false;
    g_production_enabled  = false;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 1;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
}

static void step_c1s2p2(void) {
    g_diminishing_returns = false;
    g_production_enabled  = false;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 1;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
}

static void step_c1s2p3(void) {
    g_diminishing_returns = false;
    g_production_enabled  = false;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 0;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
}

static void step_c1s2p4(void) {
    g_diminishing_returns = true;
    g_production_enabled  = false;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 0;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
}

static void step_c1s2p5(void) {
    g_diminishing_returns = true;
    g_production_enabled  = false;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 0;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
}

static void step_c1s2p6(void) {
    g_diminishing_returns = true;
    g_production_enabled  = true;
    g_leisure_enabled     = false;
    g_two_goods           = false;
    g_allow_debt          = 0;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
}

static void step_c1s2p7(void) {
    g_diminishing_returns = true;
    g_production_enabled  = true;
    g_leisure_enabled     = true;
    g_two_goods           = false;
    g_allow_debt          = 0;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
}

// ---------------------------------------------------------------------------
// Scene init functions
// ---------------------------------------------------------------------------

static void init_c1s1(SimContext *ctx) {
    ctx->sim->count = 2;
    agents_init(ctx->sim->agents, ctx->sim->count, ctx->sim->worldW, ctx->sim->worldH);
    for (int i = 0; i < ctx->sim->count; i++) {
        float t = (float)GetRandomValue(0, 10000) / 10000.0f;
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].maxUtility = 20.0f + t * 40.0f;
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].priceExpectation =
            ctx->sim->agents[i].econ.markets[MARKET_WOOD].maxUtility;
    }
}

static void init_c1s2(SimContext *ctx) {
    ctx->sim->count = 100;
    agents_init(ctx->sim->agents, ctx->sim->count, ctx->sim->worldW, ctx->sim->worldH);
    for (int i = 0; i < ctx->sim->count; i++) {
        float t = (float)GetRandomValue(0, 10000) / 10000.0f;
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].maxUtility = 20.0f + t * 40.0f;
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].priceExpectation =
            ctx->sim->agents[i].econ.markets[MARKET_WOOD].maxUtility;
    }
}

// ---------------------------------------------------------------------------
// Scene / Step data
// ---------------------------------------------------------------------------

static const SceneDef SCENES[] = {
    {
        "Two Agents", init_c1s1, 1,
        {
            {
                "Two Agents",
                "Two agents have different ideas of what a unit of wood is worth. "
                "Each time they fail to trade, they nudge their expected price "
                "toward their own valuation. Watch the prices converge over time.",
                step_c1s1p1, render_c1s1p1
            }
        }
    },
    {
        "Many Agents", init_c1s2, 7,
        {
            {
                "Many Agents",
                "With 100 agents, each with a different fixed valuation, all price "
                "expectations converge toward a shared market price through repeated "
                "gossip and failed trade attempts.",
                step_c1s2p1, render_c1s2p1
            },
            {
                "Supply & Demand",
                "The supply curve shows agents ranked by their minimum sell price. "
                "The demand curve shows agents ranked by their maximum buy price. "
                "They cross at the equilibrium price - the natural price the market converges to.",
                step_c1s2p2, render_c1s2p2
            },
            {
                "Market Collapse",
                "Without debt, buyers who run out of money can no longer trade. "
                "As money piles up in fewer hands, the market seizes. "
                "Watch the wealth distribution become extreme.",
                step_c1s2p3, render_c1s2p3
            },
            {
                "Diminishing Returns",
                "Agents now have diminishing marginal utility - each additional unit "
                "of wood is worth less than the last. This creates a natural stopping "
                "point: agents stop buying when the price exceeds their marginal value. "
                "The market stabilises.",
                step_c1s2p4, render_c1s2p4
            },
            {
                "Goods Decay",
                "Wood now rots over time. As inventory evaporates, the supply curve "
                "shrinks and prices rise. Try adjusting the decay rate to see how "
                "the market responds.",
                step_c1s2p5, render_c1s2p5
            },
            {
                "Production",
                "Agents can now chop wood. When the market price exceeds their cost "
                "of effort, they produce and sell. Adjust the chop yield to flood "
                "or starve the market.",
                step_c1s2p6, render_c1s2p6
            },
            {
                "Leisure",
                "Agents now value leisure - resting has diminishing utility too. "
                "They balance chopping, trading, and resting. "
                "Raise or lower the leisure value to see how it shifts supply.",
                step_c1s2p7, render_c1s2p7
            }
        }
    }
};

#define SCENE_COUNT ((int)(sizeof(SCENES) / sizeof(SCENES[0])))

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static const StepDef *current_step(const WalkthroughState *wt) {
    return &SCENES[wt->scene].steps[wt->step];
}

const StepDef *walkthrough_current_step(const WalkthroughState *wt) {
    return current_step(wt);
}

// Apply scene init (sets agent count + reinits agents). Called on scene entry.
static void apply_scene(WalkthroughState *wt, SimContext *ctx) {
    SCENES[wt->scene].init(ctx);
}

// Apply step init (sets feature flags only, no agent reinit).
static void apply_step(const WalkthroughState *wt) {
    const StepDef *s = current_step(wt);
    if (s->init) s->init();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

static void open_popup(WalkthroughState *wt) {
    wt->popup_active = true;
    g_sim.paused = true;
}

void walkthrough_init(WalkthroughState *wt, SimContext *ctx) {
    wt->scene = 0; wt->step = 0; wt->active = true;
    wt->scene_changed = false;
    apply_step(wt);
    apply_scene(wt, ctx);
    open_popup(wt);
}

bool walkthrough_next_step(WalkthroughState *wt, SimContext *ctx) {
    wt->scene_changed = false;
    if (wt->step + 1 < SCENES[wt->scene].stepCount) {
        wt->step++;
        apply_step(wt);
    } else if (wt->scene + 1 < SCENE_COUNT) {
        wt->scene++; wt->step = 0;
        apply_step(wt);
        apply_scene(wt, ctx);
        wt->scene_changed = true;
    } else {
        walkthrough_exit(wt);
        return false;
    }
    open_popup(wt);
    return true;
}

bool walkthrough_prev_step(WalkthroughState *wt, SimContext *ctx) {
    wt->scene_changed = false;
    if (wt->step > 0) {
        wt->step--;
        apply_step(wt);
    } else if (wt->scene > 0) {
        wt->scene--;
        wt->step = SCENES[wt->scene].stepCount - 1;
        apply_step(wt);
        apply_scene(wt, ctx);
        wt->scene_changed = true;
    } else {
        return false;
    }
    open_popup(wt);
    return true;
}

void walkthrough_restart(WalkthroughState *wt, SimContext *ctx) {
    wt->step = 0;
    apply_step(wt);
    apply_scene(wt, ctx);
    open_popup(wt);
}

void walkthrough_apply(WalkthroughState *wt, SimContext *ctx) {
    apply_step(wt);
    apply_scene(wt, ctx);
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

    // While popup is showing, only Ok/Enter/Space can dismiss it
    if (wt->popup_active) {
        bool dismiss = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
        if (!dismiss && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Ok button: centred, below text box
            int bw = 100, bh = 32;
            int bx = (SCREEN_W - bw) / 2;
            int by = SCREEN_H / 2 + 80;
            Vector2 m = GetMousePosition();
            dismiss = m.x >= bx && m.x <= bx + bw && m.y >= by && m.y <= by + bh;
        }
        if (dismiss) {
            wt->popup_active = false;
            g_sim.paused = false;
        }
        return false;
    }

    if (IsKeyPressed(KEY_SPACE)) { g_sim.paused = !g_sim.paused; return false; }

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
    if (m.x >= 10 && m.x <= 80 && m.y >= 2 && m.y <= 32) {
        walkthrough_prev_step(wt, ctx); return true;
    }
    if (m.x >= SCREEN_W-80 && m.x <= SCREEN_W-10 && m.y >= 2 && m.y <= 32) {
        walkthrough_next_step(wt, ctx); return true;
    }
    if (m.x >= SCREEN_W-170 && m.x <= SCREEN_W-90 && m.y >= 2 && m.y <= 32) {
        walkthrough_restart(wt, ctx); return true;
    }
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
    const StepDef  *s  = current_step(wt);
    const SceneDef *sc = &SCENES[wt->scene];

    DrawRectangle(0, 0, SCREEN_W, WTHROUGH_NAV_H, (Color){18, 26, 40, 240});
    DrawLine(0, WTHROUGH_NAV_H, SCREEN_W, WTHROUGH_NAV_H, (Color){70, 90, 120, 255});

    Vector2 mouse = GetMousePosition();

    bool hPrev = mouse.x>=10&&mouse.x<=80&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(10, 4, 70, WTHROUGH_NAV_H-8, hPrev?(Color){60,90,140,255}:(Color){35,55,90,255});
    DrawRectangleLines(10, 4, 70, WTHROUGH_NAV_H-8, (Color){80,120,180,200});
    DrawText("< Prev", 14, 10, 13, hPrev ? WHITE : (Color){180,190,210,255});

    bool hNext = mouse.x>=SCREEN_W-80&&mouse.x<=SCREEN_W-10&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(SCREEN_W-80, 4, 70, WTHROUGH_NAV_H-8, hNext?(Color){60,90,140,255}:(Color){35,55,90,255});
    DrawRectangleLines(SCREEN_W-80, 4, 70, WTHROUGH_NAV_H-8, (Color){80,120,180,200});
    DrawText("Next >", SCREEN_W-76, 10, 13, hNext ? WHITE : (Color){180,190,210,255});

    bool hRes = mouse.x>=SCREEN_W-170&&mouse.x<=SCREEN_W-90&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(SCREEN_W-170, 4, 70, WTHROUGH_NAV_H-8, hRes?(Color){80,50,20,255}:(Color){50,32,12,255});
    DrawRectangleLines(SCREEN_W-170, 4, 70, WTHROUGH_NAV_H-8, (Color){160,100,40,200});
    DrawText("Restart", SCREEN_W-166, 10, 13, hRes ? WHITE : (Color){180,140,80,255});

    bool hExit = mouse.x>=SCREEN_W-260&&mouse.x<=SCREEN_W-180&&mouse.y>=4&&mouse.y<=WTHROUGH_NAV_H-4;
    DrawRectangle(SCREEN_W-260, 4, 70, WTHROUGH_NAV_H-8, hExit?(Color){80,20,20,255}:(Color){50,12,12,255});
    DrawRectangleLines(SCREEN_W-260, 4, 70, WTHROUGH_NAV_H-8, (Color){160,60,60,200});
    DrawText("Exit", SCREEN_W-248, 10, 13, hExit ? WHITE : (Color){200,100,100,255});

    char label[128];
    snprintf(label, sizeof(label), "%s  -  Step %d/%d",
             sc->title, wt->step + 1, sc->stepCount);
    int lw = MeasureText(label, 12);
    DrawText(label, (SCREEN_W - lw) / 2, 6, 12, (Color){200,210,230,255});

    int stw = MeasureText(s->title, 11);
    DrawText(s->title, (SCREEN_W - stw) / 2, 21, 11, (Color){120,160,220,200});

    int textY = WTHROUGH_NAV_H + WORLD_VIEW_H - 75;
    DrawRectangle(0, textY, SCREEN_W, 75, (Color){0, 0, 0, 160});
    DrawLine(0, textY, SCREEN_W, textY, (Color){70, 90, 120, 200});
    draw_wrapped_text(s->text, 20, textY + 8, SCREEN_W - 40, 13,
                      (Color){200, 210, 230, 240});

    // Step intro popup
    if (wt->popup_active) {
        // Dim background
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){0, 0, 0, 160});

        // Popup box
        int pw = 680, ph = 240;
        int px = (SCREEN_W - pw) / 2;
        int py = SCREEN_H / 2 - ph / 2;
        DrawRectangle(px, py, pw, ph, (Color){18, 26, 40, 250});
        DrawRectangleLines(px, py, pw, ph, (Color){80, 120, 180, 255});

        // Scene + step title
        char title[128];
        snprintf(title, sizeof(title), "%s — %s", sc->title, s->title);
        int tw = MeasureText(title, 16);
        DrawText(title, (SCREEN_W - tw) / 2, py + 18, 16, (Color){180, 200, 240, 255});
        DrawLine(px + 20, py + 42, px + pw - 20, py + 42, (Color){60, 80, 120, 200});

        // Body text
        draw_wrapped_text(s->text, px + 24, py + 52, pw - 48, 14,
                          (Color){200, 210, 230, 240});

        // Ok button
        int bw = 100, bh = 32;
        int bx = (SCREEN_W - bw) / 2;
        int by = py + ph - bh - 16;
        Vector2 mouse = GetMousePosition();
        bool hover = mouse.x >= bx && mouse.x <= bx + bw &&
                     mouse.y >= by && mouse.y <= by + bh;
        DrawRectangle(bx, by, bw, bh, hover ? (Color){60,90,140,255} : (Color){35,55,90,255});
        DrawRectangleLines(bx, by, bw, bh, (Color){80,120,180,200});
        int ow = MeasureText("Ok", 15);
        DrawText("Ok", bx + (bw - ow) / 2, by + 8, 15, hover ? WHITE : (Color){180,190,210,255});
    }
}
