#include "agent.h"
#include "market.h"
#include "raylib.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Per-agent economic update — called once per agent per frame from agent.c
// ---------------------------------------------------------------------------

static void decide_action(Agent *a) {
    AgentMarket *mw = AGENT_MKT(a, MARKET_WOOD);
    AgentMarket *mc = AGENT_MKT(a, MARKET_CHAIR);

    float leisureVal = leisure_value(&a->econ.leisure);
    float upd        = utility_per_dollar(a->econ.money);

    // Chop: best of personal marginal utility or utility of selling at EMV
    float chopVal     = market_potential_value(mw);
    float chopSellUtil = mw->expectedMarketValue * upd;
    if (chopSellUtil > chopVal) chopVal = chopSellUtil;

    // Build: utility gain from new chair minus utility cost of WOOD_PER_CHAIR woods
    float buildVal = -999.0f;
    if (mw->goods >= WOOD_PER_CHAIR) {
        float chairGain     = market_potential_value(mc);
        float chairSellUtil = mc->expectedMarketValue * upd;
        if (chairSellUtil > chairGain) chairGain = chairSellUtil;
        float woodCost = market_current_value(mw) * (float)WOOD_PER_CHAIR;
        buildVal = chairGain - woodCost;
    }

    AgentAction best = ACTION_LEISURE;
    float bestVal    = leisureVal;
    if (chopVal  > bestVal) { best = ACTION_CHOP;  bestVal = chopVal; }
    if (buildVal > bestVal) { best = ACTION_BUILD; }

    a->econ.lastAction = best;
    if (best == ACTION_CHOP) {
        mw->goods++;
        a->econ.leisure.idleTime = 0.0f;
    } else if (best == ACTION_BUILD) {
        mw->goods -= WOOD_PER_CHAIR;
        mc->goods++;
        a->econ.leisure.idleTime = 0.0f;
    } else {
        a->econ.leisure.idleTime += a->econ.productionInterval;
    }
}

void econ_update(Agent *a, float dt) {
    // Goods decay
    int woods = a->econ.markets[MARKET_WOOD].goods;
    if (woods > 0 && g_wood_break_prob > 0.0f) {
        float p = g_wood_break_prob * dt * (float)woods;
        if ((float)GetRandomValue(0, 100000) * 0.00001f < p)
            a->econ.markets[MARKET_WOOD].goods--;
    }

    int chairs = a->econ.markets[MARKET_CHAIR].goods;
    if (chairs > 0 && g_chair_break_prob > 0.0f) {
        float p = g_chair_break_prob * dt * (float)chairs;
        if ((float)GetRandomValue(0, 100000) * 0.00001f < p)
            a->econ.markets[MARKET_CHAIR].goods--;
    }

    // Idle frustration: drift EMV toward personal indifference price when unable to trade.
    // Buyers drift up toward their marginal value; sellers drift down toward their floor.
    float upd = utility_per_dollar(a->econ.money);
    for (int mid = 0; mid < MARKET_COUNT; mid++) {
        AgentMarket *m = &a->econ.markets[mid];
        int couldBuy  = market_is_buyer(m, a->econ.money)
                        && (g_allow_debt || a->econ.money >= m->expectedMarketValue);
        int couldSell = market_is_seller(m, a->econ.money) && m->goods > 0;
        if (couldBuy || couldSell) {
            m->timeSinceLastTrade += dt;
            if (m->timeSinceLastTrade > m->maxTimeSinceLastTrade) {
                m->timeSinceLastTrade = 0.0f;
                // Convert utility to dollar indifference price: ₴ / (₴/$) = $
                float utilVal = couldBuy ? market_potential_value(m) : market_current_value(m);
                float signal  = utilVal / upd;
                m->expectedMarketValue = nerlove_update(m->expectedMarketValue, signal, a->econ.alpha);
            }
        }
    }

    // Production decision
    a->econ.actionTimer -= dt;
    if (a->econ.actionTimer <= 0.0f) {
        a->econ.actionTimer = a->econ.productionInterval;
        decide_action(a);
    }
}

// ---------------------------------------------------------------------------
// Batch influence helpers (Fisher-Yates partial shuffle)
// ---------------------------------------------------------------------------

static void partial_shuffle(int *indices, int count, int n) {
    for (int i = 0; i < n; i++) {
        int j   = i + GetRandomValue(0, count - 1 - i);
        int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
    }
}

void agents_influence(Agent *agents, int count, int n, float delta, MarketId mid) {
    if (n > count) n = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, n);
    for (int i = 0; i < n; i++) {
        float *pv = &agents[indices[i]].econ.markets[mid].basePersonalValue;
        *pv += delta;
        if (*pv < 1.0f) *pv = 1.0f;
    }
}

void agents_give_money(Agent *agents, int count, int n, float delta) {
    if (n > count) n = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, n);
    for (int i = 0; i < n; i++)
        agents[indices[i]].econ.money += delta;
}

void agents_give_goods(Agent *agents, int count, int n, int delta, MarketId mid) {
    if (n > count) n = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, n);
    for (int i = 0; i < n; i++) {
        int *goods = &agents[indices[i]].econ.markets[mid].goods;
        *goods += delta;
        if (*goods < 0) *goods = 0;
    }
}
