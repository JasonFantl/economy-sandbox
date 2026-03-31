#include "agent.h"
#include "market.h"
#include "raylib.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Production decision — called once per production period
// ---------------------------------------------------------------------------

static void choose_action(Agent *a) {
    AgentMarket *wood  = AGENT_MKT(a, MARKET_WOOD);
    AgentMarket *chair = AGENT_MKT(a, MARKET_CHAIR);

    float idle_util    = leisure_utility(&a->econ.leisure);
    float money_util   = money_marginal_utility(a->econ.money);

    // Chop: best of personal marginal utility or utility of selling wood at expected price
    float chop_util  = marginal_buy_utility(wood);
    float sell_wood  = wood->priceExpectation * money_util;
    if (sell_wood > chop_util) chop_util = sell_wood;

    // Build: utility gain from new chair minus utility cost of WOOD_PER_CHAIR woods
    float build_util = -999.0f;
    if (wood->goods >= WOOD_PER_CHAIR) {
        float chair_gain     = marginal_buy_utility(chair);
        float sell_chair     = chair->priceExpectation * money_util;
        if (sell_chair > chair_gain) chair_gain = sell_chair;
        float wood_cost      = marginal_sell_utility(wood) * (float)WOOD_PER_CHAIR;
        build_util = chair_gain - wood_cost;
    }

    AgentAction best     = ACTION_LEISURE;
    float       best_val = idle_util;
    if (chop_util  > best_val) { best = ACTION_CHOP;  best_val = chop_util; }
    if (build_util > best_val) { best = ACTION_BUILD; }

    a->econ.lastAction = best;
    if (best == ACTION_CHOP) {
        wood->goods++;
        a->econ.leisure.idleTime = 0.0f;
    } else if (best == ACTION_BUILD) {
        wood->goods  -= WOOD_PER_CHAIR;
        chair->goods++;
        a->econ.leisure.idleTime = 0.0f;
    } else {
        a->econ.leisure.idleTime += a->econ.productionPeriod;
    }
}

// ---------------------------------------------------------------------------
// Per-agent economic update — called every frame from agent.c
// ---------------------------------------------------------------------------

void agent_econ_update(Agent *a, float dt) {
    // Goods decay
    int woods = a->econ.markets[MARKET_WOOD].goods;
    if (woods > 0 && g_wood_decay_rate > 0.0f) {
        float p = g_wood_decay_rate * dt * (float)woods;
        if ((float)GetRandomValue(0, 100000) * 0.00001f < p)
            a->econ.markets[MARKET_WOOD].goods--;
    }

    int chairs = a->econ.markets[MARKET_CHAIR].goods;
    if (chairs > 0 && g_chair_decay_rate > 0.0f) {
        float p = g_chair_decay_rate * dt * (float)chairs;
        if ((float)GetRandomValue(0, 100000) * 0.00001f < p)
            a->econ.markets[MARKET_CHAIR].goods--;
    }

    // Frustration: when unable to trade, drift price expectation toward own indifference price.
    // Buyers raise their offer; sellers lower their ask.
    float money_util = money_marginal_utility(a->econ.money);
    for (int mid = 0; mid < MARKET_COUNT; mid++) {
        AgentMarket *m        = &a->econ.markets[mid];
        int          willing_buyer = wants_to_buy(m, a->econ.money)
                                     && (g_allow_debt || a->econ.money >= m->priceExpectation);
        int          willing_seller = wants_to_sell(m, a->econ.money) && m->goods > 0;
        if (willing_buyer || willing_seller) {
            m->frustrationTimer += dt;
            if (m->frustrationTimer > m->frustrationThreshold) {
                m->frustrationTimer = 0.0f;
                // Indifference price = personal utility / money_marginal_utility (₴ / ₴/$ = $)
                float indifference_price = (willing_buyer
                                             ? marginal_buy_utility(m)
                                             : marginal_sell_utility(m))
                                           / money_util;
                m->priceExpectation = nerlove_update(m->priceExpectation,
                                                     indifference_price,
                                                     a->econ.beliefUpdateRate);
            }
        }
    }

    // Production decision
    a->econ.productionTimer -= dt;
    if (a->econ.productionTimer <= 0.0f) {
        a->econ.productionTimer = a->econ.productionPeriod;
        choose_action(a);
    }
}

// ---------------------------------------------------------------------------
// Batch injection helpers (Fisher-Yates partial shuffle)
// ---------------------------------------------------------------------------

static void partial_shuffle(int *indices, int count, int n) {
    for (int i = 0; i < n; i++) {
        int j   = i + GetRandomValue(0, count - 1 - i);
        int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
    }
}

void agents_adjust_valuations(Agent *agents, int count, int numAgents, float delta, MarketId mid) {
    if (numAgents > count) numAgents = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, numAgents);
    for (int i = 0; i < numAgents; i++) {
        float *v = &agents[indices[i]].econ.markets[mid].maxUtility;
        *v += delta;
        if (*v < 1.0f) *v = 1.0f;
    }
}

void agents_inject_money(Agent *agents, int count, int numAgents, float delta) {
    if (numAgents > count) numAgents = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, numAgents);
    for (int i = 0; i < numAgents; i++)
        agents[indices[i]].econ.money += delta;
}

void agents_inject_goods(Agent *agents, int count, int numAgents, int delta, MarketId mid) {
    if (numAgents > count) numAgents = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, numAgents);
    for (int i = 0; i < numAgents; i++) {
        int *goods = &agents[indices[i]].econ.markets[mid].goods;
        *goods += delta;
        if (*goods < 0) *goods = 0;
    }
}
