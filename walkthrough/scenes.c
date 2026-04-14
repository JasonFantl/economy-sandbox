#include "walkthrough/scenes.h"
#include "econ/agent.h"
#include <stddef.h>
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
                         CX(x), CY(y), CW(hw), CH(h), NULL);
    draw_plot_frame(x + hw + gap, y, hw, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+hw+gap), CY(y), CW(hw), CH(h), false, NULL);
    WT_INF(ctx, 0, 10);
}

static void render_s2p1(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, hw = (w - gap) / 2;
    draw_plot_frame(x, y, hw, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x), CY(y), CW(hw), CH(h), NULL);
    draw_plot_frame(x + hw + gap, y, hw, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+hw+gap), CY(y), CW(hw), CH(h), false, NULL);
    WT_INF(ctx, 0, 10);
}

static void render_s2p2(const SimContext *ctx, int x, int y, int w, int h) {
    int gap = 8, w3 = (w - 2*gap) / 3;
    draw_plot_frame(x, y, w3, h);
    panel_supply_demand(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                        CX(x), CY(y), CW(w3), CH(h));
    draw_plot_frame(x + w3+gap, y, w3, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x+w3+gap), CY(y), CW(w3), CH(h), NULL);
    draw_plot_frame(x + 2*(w3+gap), y, w3, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+2*(w3+gap)), CY(y), CW(w3), CH(h), false, NULL);
    WT_INF(ctx, WT_INF_WOOD_VALUE, 10);
}

// 3-in-a-row shared by s2p3–s2p8: Wealth | Val Dist | Price History
// cfg: if non-NULL, axis selector buttons appear on the wealth plot (from s2p6 onward)
// yboxVal/yboxPrice: if non-NULL, editable Y-max boxes appear on those plots (from s2p7 onward)
static void render_three_panels(const SimContext *ctx, int x, int y, int w, int h,
                                 bool showUtil, WealthAxisConfig *cfg,
                                 YRangeBox *yboxVal, YRangeBox *yboxPrice) {
    int gap = 8, w3 = (w - 2*gap) / 3;
    draw_plot_frame(x, y, w3, h);
    panel_wealth(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                 CX(x), CY(y), CW(w3), CH(h), cfg);
    draw_plot_frame(x + w3+gap, y, w3, h);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x+w3+gap), CY(y), CW(w3), CH(h), yboxVal);
    draw_plot_frame(x + 2*(w3+gap), y, w3, h);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+2*(w3+gap)), CY(y), CW(w3), CH(h), showUtil, yboxPrice);
}

static void render_s2p3(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, false, NULL, NULL, NULL);
    WT_INF(ctx, WT_INF_WOOD_VALUE, 10);
}

static void render_s2p4(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, true, NULL, NULL, NULL);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_DIM_RETURNS, 10);
}

static void render_s2p5(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, true, NULL, NULL, NULL);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_DIM_RETURNS, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY, 258);
}

static void render_s2p6(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, true, ctx->wt_wealth, NULL, NULL);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_DIM_RETURNS, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY | WT_ENV_CHOP_YIELD, 258);
}

static void render_s2p7(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, true, ctx->wt_wealth,
                        &ctx->wt_ybox_val[MARKET_WOOD], &ctx->wt_ybox_price[MARKET_WOOD]);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_DIM_RETURNS | WT_INF_INFLATION | WT_INF_MONEY, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY | WT_ENV_CHOP_YIELD, 258);
}

static void render_s2p8(const SimContext *ctx, int x, int y, int w, int h) {
    render_three_panels(ctx, x, y, w, h, true, ctx->wt_wealth,
                        &ctx->wt_ybox_val[MARKET_WOOD], &ctx->wt_ybox_price[MARKET_WOOD]);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_DIM_RETURNS | WT_INF_LEISURE | WT_INF_INFLATION | WT_INF_MONEY, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY | WT_ENV_CHOP_YIELD, 258);
}

// 3×2 grid for scene 3: top row = wood, bottom row = chair
static void render_s3_plots(const SimContext *ctx, int x, int y, int w, int h, bool showUtil,
                             WealthAxisConfig *cfgWood, WealthAxisConfig *cfgChair,
                             YRangeBox *yboxVal, YRangeBox *yboxPrice) {
    int gap = 8, w3 = (w - 2*gap) / 3, hh = (h - gap) / 2;
    // Top row: Wood
    draw_plot_frame(x,            y, w3, hh);
    panel_wealth(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                 CX(x), CY(y), CW(w3), CH(hh), cfgWood);
    draw_plot_frame(x + w3+gap,   y, w3, hh);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_WOOD,
                         CX(x+w3+gap), CY(y), CW(w3), CH(hh),
                         yboxVal ? &yboxVal[MARKET_WOOD] : NULL);
    draw_plot_frame(x + 2*(w3+gap), y, w3, hh);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_WOOD, CX(x+2*(w3+gap)), CY(y), CW(w3), CH(hh), showUtil,
                        yboxPrice ? &yboxPrice[MARKET_WOOD] : NULL);
    // Bottom row: Chair
    draw_plot_frame(x,            y+hh+gap, w3, hh);
    panel_wealth(ctx->sim->agents, ctx->sim->count, MARKET_CHAIR,
                 CX(x), CY(y+hh+gap), CW(w3), CH(hh), cfgChair);
    draw_plot_frame(x + w3+gap,   y+hh+gap, w3, hh);
    panel_valuation_dist(ctx->sim->agents, ctx->sim->count, MARKET_CHAIR,
                         CX(x+w3+gap), CY(y+hh+gap), CW(w3), CH(hh),
                         yboxVal ? &yboxVal[MARKET_CHAIR] : NULL);
    draw_plot_frame(x + 2*(w3+gap), y+hh+gap, w3, hh);
    panel_price_history(ctx->sim->avh, ctx->sim->pvh, ctx->sim->agents, ctx->sim->count,
                        MARKET_CHAIR, CX(x+2*(w3+gap)), CY(y+hh+gap), CW(w3), CH(hh), showUtil,
                        yboxPrice ? &yboxPrice[MARKET_CHAIR] : NULL);
}

static void render_s3p1(const SimContext *ctx, int x, int y, int w, int h) {
    render_s3_plots(ctx, x, y, w, h, true, ctx->wt_wealth, ctx->wt_wealth_chair,
                    ctx->wt_ybox_val, ctx->wt_ybox_price);
    WT_INF(ctx, WT_INF_WOOD_VALUE | WT_INF_WOOD_COUNT | WT_INF_DIM_RETURNS | WT_INF_LEISURE | WT_INF_MARKET_SEL | WT_INF_INFLATION | WT_INF_MONEY, 10);
    WT_ENV(ctx, WT_ENV_WOOD_DECAY | WT_ENV_DECAY_MARKET_SEL | WT_ENV_CHOP_YIELD | WT_ENV_BUILD_COST, 258);
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
    g_leisure_enabled     = false;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
    g_inflation_enabled   = true;
}

static void step_s2p8(void) {
    g_diminishing_returns = true;
    g_chop_wood_enabled    = true;
    g_leisure_enabled     = true;
    g_build_chairs_enabled = false;
    g_disable_executing_trade = false;
    g_wood_decay_rate     = 0.003f;
    g_chair_decay_rate    = 0.003f;
    g_inflation_enabled   = true;
}

static void step_s3p1(void) {
    g_diminishing_returns    = true;
    g_chop_wood_enabled      = true;
    g_leisure_enabled        = true;
    g_build_chairs_enabled   = true;
    g_disable_executing_trade = false;
    g_wood_decay_rate        = 0.003f;
    g_chair_decay_rate       = 0.003f;
    g_inflation_enabled      = true;
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

    // Three clusters with random relative sizes.
    // Weights are drawn independently then normalised to cumulative thresholds.
    float w1 = (float)GetRandomValue(10, 60);
    float w2 = (float)GetRandomValue(10, 60);
    float w3 = (float)GetRandomValue(10, 60);
    float total = w1 + w2 + w3;
    float p1 = w1 / total;           // threshold for cluster 1
    float p2 = (w1 + w2) / total;    // threshold for cluster 2

    for (int i = 0; i < ctx->sim->count; i++) {
        // Triangular distribution within each cluster (average of two uniforms)
        float a = (float)GetRandomValue(0, 10000) / 10000.0f;
        float b = (float)GetRandomValue(0, 10000) / 10000.0f;
        float t = (a + b) * 0.5f;

        float roll = (float)GetRandomValue(0, 10000) / 10000.0f;
        float value;
        if (roll < p1)
            value = 20.0f + t * 14.0f;  // low cluster:  ~20–34, peak ~27
        else if (roll < p2)
            value = 33.0f + t * 14.0f;  // mid cluster:  ~33–47, peak ~40
        else
            value = 46.0f + t * 14.0f;  // high cluster: ~46–60, peak ~53

        ctx->sim->agents[i].econ.markets[MARKET_WOOD].maxUtility      = value;
        ctx->sim->agents[i].econ.markets[MARKET_WOOD].priceExpectation = value;
    }
}

static void init_s3(SimContext *ctx) {
    // Agents carry over from scene 2 (count, money, wood state all preserved).
    // Assign a fresh chair valuation distribution.
    float cw1 = (float)GetRandomValue(10, 60);
    float cw2 = (float)GetRandomValue(10, 60);
    float cw3 = (float)GetRandomValue(10, 60);
    float ct  = cw1 + cw2 + cw3;
    float cp1 = cw1 / ct, cp2 = (cw1 + cw2) / ct;

    for (int i = 0; i < ctx->sim->count; i++) {
        float a = (float)GetRandomValue(0, 10000) / 10000.0f;
        float b = (float)GetRandomValue(0, 10000) / 10000.0f;
        float t = (a + b) * 0.5f;
        float roll = (float)GetRandomValue(0, 10000) / 10000.0f;
        float value;
        if      (roll < cp1) value =  20.0f + t * 60.0f;  // low cluster:  ~20–80,  peak ~50
        else if (roll < cp2) value =  80.0f + t * 60.0f;  // mid cluster:  ~80–140, peak ~110
        else                 value = 140.0f + t * 60.0f;  // high cluster: ~140–200, peak ~170
        ctx->sim->agents[i].econ.markets[MARKET_CHAIR].maxUtility      = value;
        ctx->sim->agents[i].econ.markets[MARKET_CHAIR].priceExpectation = value;
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
                "Every agent has a Utility for a good, which is the blue dot. They also have an expected Market Price for a good. Every time two agents try to trade, they update their expected market price. "
                "If a trade is successful, each agent will try to "
                "get a better deal the next time: buyers offer less (lower expected Market Price), sellers ask for more (higher expected Market Price). But if a deal fails, then agents update in the other direction, offering a better deal next time.\n"
                "Hit F to speed up the simulation, Shift+F to slow down, and Space to pause.",
                step_s1p1, render_s1p1
            }
        }
    },
    {
        "Many Agents", init_s2, 8,
        {
            {
                "Many Agents",
                "Now we have 100 agents all randomly interacting with each other. Even though "
                "they all have different Utility for the good and start with a different expected Market Price, they eventually all converge on a shared Market Price.\n"
                "A green Market Price indicates the agent is a Buyer, while red indicates the agent is a Seller.\n"
                "Try resetting the agents in the top left corner to see how a different distribution of utility values leads to a different Market Price.",
                step_s2p1, render_s2p1
            },
            {
                "Supply & Demand",
                "Notice that if we add a Supply & Demand curve, our market has converged to the "
                "equilibrium price.\n"
                "There is a button to add or subtract value from every agent in the Agents panel, "
                "try updating it and watch how the market slowly updates.",
                step_s2p2, render_s2p2
            },
            {
                "Market Collapse",
                "But we need to give agents money and goods to actually trade with. We see that everyone "
                "either sells all their goods or spends all their money buying goods. "
                "Why don't people actually do this in real life?\n"
                "A Seller without any goods is now colored grey, since they can no longer sell.",
                step_s2p3, render_s2p3
            },
            {
                "Diminishing Returns",
                "Agents now have diminishing utility - the more of a good they have, the less "
                "they value it. On the value plot we now see the buy and sell utility calculated "
                "from how many goods the agent holds (buying utility is lower than selling since "
                "the next unit you buy is not as valuable as the one you would give up).\n"
                "An agent will be colored grey if they are no longer a Buyer or Seller.\n"
                "Try adding goods to agents' inventories in the panel and see that they value the good less.",
                step_s2p4, render_s2p4
            },
            {
                "Goods Decay",
                "But now our good will decay over time. Let's say that the good is wood logs, which are rotting over time. This causes the price to increase until all the wood in the market eventually disappears.\n"
                "You can control the rate of decay in the Environment panel. As a reminder, you can reset agents in the Agents panel.",
                step_s2p5, render_s2p5
            },
            {
                "Production",
                "Now agents can chop down new wood, which we indicate with a brown color in the Goods V Wealth plot. At some point there is enough wood that the rate of decay is equal to rate of new wood and they balance. "
                "Play with the Decay rate and Yield to find where they "
                "balance.\n"
                "If you speed up the simulation you will notice that some agents keep getting "
                "richer while others get poorer. Why might this be?\n"
                "Play with changing the axis in the Goods V Wealth plot to identify patterns.",
                step_s2p6, render_s2p6
            },
            {
                "Inflation",
                "So far agents buy and sell based purely on the utility of the good, with no "
                "regard for how much money they have. In reality, money itself has diminishing "
                "value — a dollar means more to a poor person than a rich one.\n"
                "Inflation is now enabled: each agent's willingness to trade is weighted by the "
                "marginal utility of money at their current wealth. Wealthier agents accept "
                "lower returns on their money, making them more willing to buy. Toggle inflation "
                "off in the Agents panel to compare the two behaviours.",
                step_s2p7, render_s2p7
            },
            {
                "Leisure",
                "Now agents also consider the value of leisure instead of chopping wood all the "
                "time. They choose whichever action is worth the most to them. Set the leisure "
                "value to see how it affects the price and the amount of goods in the market.\n"
                "Notice the interesting behaviour when you set leisure high and then low: the "
                "number of goods initially spikes and then comes back down once the price "
                "corrects.",
                step_s2p8, render_s2p8
            }
        }
    },
    {
        "Two Goods", init_s3, 1,
        {
            {
                "Production",
                "Agents can now build chairs from wood. It costs a certain amount of wood "
                "to build one chair. This links the two markets: as agents consume wood to "
                "build chairs, the wood supply shrinks and the chair supply grows.\n"
                "Use the Build cost setter in the Environment panel to change how much wood "
                "a chair costs, and watch how it shifts both markets.",
                step_s3p1, render_s3p1
            }
        }
    }
};

const int SCENE_COUNT = (int)(sizeof(SCENES) / sizeof(SCENES[0]));
