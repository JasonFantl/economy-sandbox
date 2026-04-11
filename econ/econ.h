#ifndef ECON_H
#define ECON_H

#include "econ/agent.h"
#include <math.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Feature flags (defined in econ.c)
// ---------------------------------------------------------------------------
extern bool g_diminishing_returns;  // false = flat utility curve
extern bool  g_chop_wood_enabled;    // false = agents cannot chop wood
extern bool  g_build_chairs_enabled; // false = agents cannot build chairs (chair market inactive)
extern bool  g_leisure_enabled;      // false = leisure_utility always returns 0
extern bool  g_inflation_enabled;    // true = money utility diminishes with wealth

// ---------------------------------------------------------------------------
// Inline utility / value functions
// ---------------------------------------------------------------------------

// Marginal utility of buying one more unit (willingness to pay, in utility ₴)
static inline float marginal_buy_utility(const AgentMarket *m) {
    if (!g_diminishing_returns) return m->maxUtility;
    float r = (float)(m->goods + 1) / m->halfSaturation;
    return m->minUtility + (m->maxUtility - m->minUtility) / (r * r * r + 1.0f);
}

// Marginal utility of the last unit owned (minimum acceptable sell price, in utility ₴)
static inline float marginal_sell_utility(const AgentMarket *m) {
    if (!g_diminishing_returns) return m->maxUtility;
    float r = (float)m->goods / m->halfSaturation;
    return m->minUtility + (m->maxUtility - m->minUtility) / (r * r * r + 1.0f);
}

// Leisure utility (constant per agent)
static inline float leisure_utility(float u) {
    return g_leisure_enabled ? u : 0.0f;
}

// Marginal utility of $1 given the agent's current wealth.
// With inflation enabled, utility diminishes with wealth (richer agents value money less).
// Without inflation, money has flat constant utility.
static inline float money_marginal_utility(float money) {
    if (g_inflation_enabled) return MONEY_MAX_UTILITY / (money + 1.0f);
    return 1.0f;
}


// ---------------------------------------------------------------------------
// econ.c — economic update and batch helpers
// ---------------------------------------------------------------------------

// Exact wood gained per chop action (agents know this value when making decisions)
extern int g_chop_yield;
extern int g_wood_per_chair;

void agent_econ_update(Agent *a);
void agent_execute_chop(Agent *a);   // adds 1..g_chop_yield wood
void agent_execute_build(Agent *a);  // spends WOOD_PER_CHAIR wood, gains 1 chair

void agents_adjust_valuations(Agent *agents, int count, int numAgents, float delta, MarketId mid);
void agents_adjust_leisure(Agent *agents, int count, int numAgents, float delta);
void agents_set_leisure(Agent *agents, int count, float value);
void agents_inject_money(Agent *agents, int count, int numAgents, float delta);
void agents_inject_goods(Agent *agents, int count, int numAgents, int delta, MarketId mid);

#endif
