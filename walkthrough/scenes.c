#include "walkthrough/scenes.h"
#include "econ/agent.h"
#include "econ/econ.h"
#include "econ/market.h"
#include "render/panels.h"
#include "render/controls.h"
#include "sim.h"
#include "raylib.h"

// ---------------------------------------------------------------------------
// Step render functions
// ---------------------------------------------------------------------------

// Shorthands for walkthrough panel render calls
#define WT_INF(ctx, flags, px) \
    wt_influence_panel_render((ctx)->wt_inf, (ctx)->sim->agents, (ctx)->sim->count, flags, px)
#define WT_ENV(ctx, flags, px) \
    wt_environment_panel_render((ctx)->wt_env, flags, px)

// Plot frame: background + border drawn around the full cell (fx,fy,fw,fh).
// Content (px,py,pw,ph) is inset by asymmetric margins that keep axis text inside the border:
//   left  (PL=34): y-axis labels sit at px-28 and tick marks at px-4
//   bottom (PB=22): x-axis labels sit at py+ph+6, font height ~11px
//   top/right (PT/PR=6): small breathing room
#define PL 34
#define PT  6
#define PR  6
#define PB 22
// Given a frame at (fx,fy,fw,fh):
//   content x  = fx+PL,        content y  = fy+PT
//   content w  = fw-PL-PR,     content h  = fh-PT-PB
static const Color FRAME_BG     = {10, 16, 26, 255};
static const Color FRAME_BORDER = {55, 75, 105, 220};
static void draw_plot_frame(int px, int py, int pw, int ph) {
    DrawRectangle(px, py, pw, ph, FRAME_BG);
    DrawRectangleLines(px, py, pw, ph, FRAME_BORDER);
}
// Helpers: inset a frame rect to its content rect
#define CX(fx)       ((fx)+PL)
#define CY(fy)       ((fy)+PT)
#define CW(fw)       ((fw)-PL-PR)
#define CH(fh)       ((fh)-PT-PB)

static void render_s1p1(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    draw_plot_frame(x, y, hw, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x), CY(y), CW(hw), CH(h), 0.0f);
    draw_plot_frame(x + hw + gap, y, hw, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+hw+gap), CY(y), CW(hw), CH(h), 0.0f, false);
    WT_INF(ctx, 0, 10);
}

static void render_s2p1(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    draw_plot_frame(x, y, hw, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x), CY(y), CW(hw), CH(h), 0.0f);
    draw_plot_frame(x + hw + gap, y, hw, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+hw+gap), CY(y), CW(hw), CH(h), 0.0f, false);
    WT_INF(ctx, 0, 10);
}

static void render_s2p2(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, w3 = (w - 2*gap) / 3;
    draw_plot_frame(x, y, w3, h);
    panel_supply_demand(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                        CX(x), CY(y), CW(w3), CH(h));
    draw_plot_frame(x + w3+gap, y, w3, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x+w3+gap), CY(y), CW(w3), CH(h), 0.0f);
    draw_plot_frame(x + 2*(w3+gap), y, w3, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+2*(w3+gap)), CY(y), CW(w3), CH(h), 0.0f, false);
    WT_INF(ctx, WT_INF_WOOD_VALUE, 10);
}

// 3-in-a-row shared by s2p3–s2p7: Wealth | Val Dist | Price History
static void render_three_panels(const SimContext *ctx, int x, int y, int w, int h,
                                 float eq, bool showUtil) {
    int gap = 8, w3 = (w - 2*gap) / 3;
    draw_plot_frame(x, y, w3, h);
    panel_wealth(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                 CX(x), CY(y), CW(w3), CH(h));
    draw_plot_frame(x + w3+gap, y, w3, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x+w3+gap), CY(y), CW(w3), CH(h), eq);
    draw_plot_frame(x + 2*(w3+gap), y, w3, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+2*(w3+gap)), CY(y), CW(w3), CH(h), eq, showUtil);
}

static void render_s2p3(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, 0.0f, false);
    WT_INF(ctx, WT_INF_WOOD_VALUE, 10);
}

static float avg_eq(const SimContext *ctx, int mid) {
    float sum = 0.0f;
    for (int i = 0; i < ctx->sim->count; i++)
        sum += ctx->sim->agents[i].econ.markets[mid].maxUtility;
    return ctx->sim->count > 0 ? sum / (float)ctx->sim->count : 0.0f;
}

static void render_s2p4(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, avg_eq(ctx, MARKET_WOOD), true);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT, 10);
}

static void render_s2p5(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, avg_eq(ctx, MARKET_WOOD), true);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY, 228);
}

static void render_s2p6(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, avg_eq(ctx, MARKET_WOOD), true);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY | WT_ENV_CHOP_YIELD, 228);
}

static void render_s2p7(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, avg_eq(ctx, MARKET_WOOD), true);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_LEISURE, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY | WT_ENV_CHOP_YIELD, 228);
}

// ---------------------------------------------------------------------------
// Step init functions
// ---------------------------------------------------------------------------

static void step_s1p1(void) {
    g_diminishing_returns = false;
    g_chop_wood_enabled    = false;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = true;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
    g_inflation_enabled   = false;
}

static void step_s2p1(void) {
    g_diminishing_returns = false;
    g_chop_wood_enabled    = false;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = true;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
    g_inflation_enabled   = false;
}

static void step_s2p2(void) {
    g_diminishing_returns = false;
    g_chop_wood_enabled    = false;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = true;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
    g_inflation_enabled   = false;
}

static void step_s2p3(void) {
    g_diminishing_returns = false;
    g_chop_wood_enabled    = false;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
    g_inflation_enabled   = false;
}

static void step_s2p4(void) {
    g_diminishing_returns = true;
    g_chop_wood_enabled    = false;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.0f;
    g_chair_decay_rate    = 0.0f;
    g_inflation_enabled   = false;
}

static void step_s2p5(void) {
    g_diminishing_returns = true;
    g_chop_wood_enabled    = false;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
    g_inflation_enabled   = false;
}

static void step_s2p6(void) {
    g_diminishing_returns = true;
    g_chop_wood_enabled    = true;
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
    g_inflation_enabled   = false;
}

static void step_s2p7(void) {
    g_diminishing_returns = true;
    g_chop_wood_enabled    = true;
    g_leisure_enabled     = true;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
    g_inflation_enabled   = false;
}

// ---------------------------------------------------------------------------
// Scene init functions
// ---------------------------------------------------------------------------

static void init_s1(SimContext *ctx) {
    static const float values[2] = { 20.0f, 60.0f };
    ctx->sim->count = 2;
    agents_init(ctx->sim->agents, ctx->sim->count, ctx->sim->worldW, ctx->sim->worldH);
    for (int i = 0; i < ctx->sim->count; i++) {
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].maxUtility      = values[i];
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].priceExpectation = values[i];
    }
}

static void init_s2(SimContext *ctx) {
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

const SceneDef SCENES[] = {
    {
        "Two Agents", init_s1, 1,
        {
            {
                "Two Agents",
                "Two agents have different ideas of what a unit of wood is worth. "
                "Each time they fail to trade, they nudge their expected price "
                "toward their own valuation. Watch the prices converge over time.",
                step_s1p1, render_s1p1
            }
        }
    },
    {
        "Many Agents", init_s2, 7,
        {
            {
                "Many Agents",
                "With 100 agents, each with a different fixed valuation, all price "
                "expectations converge toward a shared market price through repeated "
                "gossip and failed trade attempts.",
                step_s2p1, render_s2p1
            },
            {
                "Supply & Demand",
                "The supply curve shows agents ranked by their minimum sell price. "
                "The demand curve shows agents ranked by their maximum buy price. "
                "They cross at the equilibrium price - the natural price the market converges to.",
                step_s2p2, render_s2p2
            },
            {
                "Market Collapse",
                "Without debt, buyers who run out of money can no longer trade. "
                "As money piles up in fewer hands, the market seizes. "
                "Watch the wealth distribution become extreme.",
                step_s2p3, render_s2p3
            },
            {
                "Diminishing Returns",
                "Agents now have diminishing marginal utility - each additional unit "
                "of wood is worth less than the last. This creates a natural stopping "
                "point: agents stop buying when the price exceeds their marginal value. "
                "The market stabilises.",
                step_s2p4, render_s2p4
            },
            {
                "Goods Decay",
                "Wood now rots over time. As inventory evaporates, the supply curve "
                "shrinks and prices rise. Try adjusting the decay rate to see how "
                "the market responds.",
                step_s2p5, render_s2p5
            },
            {
                "Production",
                "Agents can now chop wood. When the market price exceeds their cost "
                "of effort, they produce and sell. Adjust the chop yield to flood "
                "or starve the market.",
                step_s2p6, render_s2p6
            },
            {
                "Leisure",
                "Agents now value leisure - resting has diminishing utility too. "
                "They balance chopping, trading, and resting. "
                "Raise or lower the leisure value to see how it shifts supply.",
                step_s2p7, render_s2p7
            }
        }
    }
};

const int SCENE_COUNT = (int)(sizeof(SCENES) / sizeof(SCENES[0]));
