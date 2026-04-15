#include "econ/econ.h"
#include "raylib.h"
#include <stdbool.h>

int g_chop_yield    = 1;
int g_wood_per_chair = 4;

// ---------------------------------------------------------------------------
// Feature flags
// ---------------------------------------------------------------------------
bool g_diminishing_returns = true;
bool  g_chop_wood_enabled    = true;
bool  g_build_chairs_enabled = true;
bool  g_leisure_enabled      = true;
bool  g_inflation_enabled    = false;

// ---------------------------------------------------------------------------
// Production decision — called once per production period
// ---------------------------------------------------------------------------

static void choose_action(Agent *a) {
    AgentMarket *wood  = AGENT_MKT(a, MARKET_WOOD);
    AgentMarket *chair = AGENT_MKT(a, MARKET_CHAIR);

    Utility idle_utility  = leisure_utility(a->econ.leisureUtility);
    Utility money_utility = money_marginal_utility(a->econ.money);

    Utility chop_utility = -999.0f;
    if (g_chop_wood_enabled) {
        chop_utility = marginal_buy_utility(wood) * (float)g_chop_yield;
        Utility sell_wood_utility = wood->priceExpectation * money_utility * (float)g_chop_yield;
        if (sell_wood_utility > chop_utility) chop_utility = sell_wood_utility;
    }

    Utility build_utility = -999.0f;
    if (g_build_chairs_enabled && wood->goods >= g_wood_per_chair) {
        Utility chair_gain_utility = marginal_buy_utility(chair);
        Utility sell_chair_utility = chair->priceExpectation * money_utility;
        if (sell_chair_utility > chair_gain_utility) chair_gain_utility = sell_chair_utility;
        build_utility = chair_gain_utility - marginal_sell_utility(wood) * (float)g_wood_per_chair;
    }

    AgentAction best          = ACTION_LEISURE;
    Utility     best_utility  = idle_utility;
    if (chop_utility  > best_utility) { best = ACTION_CHOP;  best_utility = chop_utility; }
    if (build_utility > best_utility) { best = ACTION_BUILD; }

    if (best == ACTION_LEISURE) {
        a->econ.lastAction = ACTION_LEISURE;
    } else {
        if (a->econ.pendingWork == ACTION_LEISURE)
            a->econ.pendingWork = best;
    }
}

// ---------------------------------------------------------------------------
// Work-site execution — called on arrival at the physical work site
// ---------------------------------------------------------------------------

void agent_execute_chop(Agent *a) {
    AgentMarket *wood = AGENT_MKT(a, MARKET_WOOD);
    wood->goods += g_chop_yield;
    a->econ.lastAction = ACTION_CHOP;
}

void agent_execute_build(Agent *a) {
    AgentMarket *wood  = AGENT_MKT(a, MARKET_WOOD);
    AgentMarket *chair = AGENT_MKT(a, MARKET_CHAIR);
    if (wood->goods >= g_wood_per_chair) {
        wood->goods  -= g_wood_per_chair;
        chair->goods++;
        a->econ.lastAction = ACTION_BUILD;
    } else {
        // Wood decayed before arrival — treat as leisure
        a->econ.lastAction = ACTION_LEISURE;
    }
}

// ---------------------------------------------------------------------------
// Per-agent economic update — called every frame from agent.c
// ---------------------------------------------------------------------------

void agent_econ_update(Agent *a) {
    // Goods decay — rates are per-second; convert to per-tick probability
    int woods = a->econ.markets[MARKET_WOOD].goods;
    if (woods > 0 && g_wood_decay_rate > 0.0f) {
        float p = g_wood_decay_rate / (float)TICKS_PER_SECOND * (float)woods;
        if ((float)GetRandomValue(0, 100000) * 0.00001f < p)
            a->econ.markets[MARKET_WOOD].goods--;
    }

    int chairs = a->econ.markets[MARKET_CHAIR].goods;
    if (chairs > 0 && g_chair_decay_rate > 0.0f) {
        float p = g_chair_decay_rate / (float)TICKS_PER_SECOND * (float)chairs;
        if ((float)GetRandomValue(0, 100000) * 0.00001f < p)
            a->econ.markets[MARKET_CHAIR].goods--;
    }

    // Production decision
    a->econ.productionTick--;
    if (a->econ.productionTick <= 0) {
        a->econ.productionTick = a->econ.productionPeriodTicks;
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

void agents_adjust_valuations(Agent *agents, int count, int numAgents, Utility delta, MarketId mid) {
    if (numAgents > count) numAgents = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, numAgents);
    for (int i = 0; i < numAgents; i++) {
        Utility *v = &agents[indices[i]].econ.markets[mid].maxUtility;
        *v += delta;
        if (*v < 1.0f) *v = 1.0f;
    }
}

void agents_inject_money(Agent *agents, int count, int numAgents, Price delta) {
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

void agents_set_leisure(Agent *agents, int count, Utility value) {
    if (value < 1.0f) value = 1.0f;
    for (int i = 0; i < count; i++)
        agents[i].econ.leisureUtility = value;
}

void agents_set_belief_rate(Agent *agents, int count, float value) {
    if (value < 0.001f) value = 0.001f;
    if (value > 1.0f)   value = 1.0f;
    for (int i = 0; i < count; i++)
        agents[i].econ.beliefUpdateRate = value;
}

void agents_adjust_leisure(Agent *agents, int count, int numAgents, Utility delta) {
    if (numAgents > count) numAgents = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, numAgents);
    for (int i = 0; i < numAgents; i++) {
        Utility *v = &agents[indices[i]].econ.leisureUtility;
        *v += delta;
        if (*v < 1.0f) *v = 1.0f;
    }
}
