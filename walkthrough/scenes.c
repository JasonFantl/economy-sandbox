#include "walkthrough/scenes.h"
#include "econ/agent.h"
#include "econ/econ.h"
#include "render/panels.h"
#include "render/controls.h"
#include "raylib.h"

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
    panel_ctrl_influence_render(ctx->inf, ctx->sim->agents, ctx->sim->count, 0, 0);
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

const SceneDef SCENES[] = {
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

const int SCENE_COUNT = (int)(sizeof(SCENES) / sizeof(SCENES[0]));
