#ifndef ECON_H
#define ECON_H

#include "agent_types.h"
#include <math.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Feature flags (defined in econ.c)
// ---------------------------------------------------------------------------
extern bool g_diminishing_returns;  // false = flat utility curve
extern bool g_production_enabled;   // false = agents only choose leisure
extern bool g_leisure_enabled;      // false = leisure_utility always returns 0
extern bool g_two_goods;            // false = chair market inactive

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

// Leisure utility (diminishes the longer the agent has been idle)
static inline float leisure_utility(const LeisureState *l) {
    if (!g_leisure_enabled) return 0.0f;
    float r = l->idleTime / l->halfSaturation;
    return l->minUtility + (l->maxUtility - l->minUtility) / (r * r * r + 1.0f);
}

// Marginal utility of $1 given the agent's current wealth
static inline float money_marginal_utility(float money) {
    return MONEY_MAX_UTILITY / (money + 1.0f);
}

// True if buying would yield more utility than spending the money elsewhere
static inline int wants_to_buy(const AgentMarket *m, float money) {
    return marginal_buy_utility(m) > m->priceExpectation * money_marginal_utility(money);
}

// True if selling would yield more utility than keeping the good
static inline int wants_to_sell(const AgentMarket *m, float money) {
    return marginal_sell_utility(m) < m->priceExpectation * money_marginal_utility(money);
}

// ---------------------------------------------------------------------------
// econ.c — economic update and batch helpers
// ---------------------------------------------------------------------------

// Exact wood gained per chop action (agents know this value when making decisions)
extern int g_chop_yield;

void agent_econ_update(Agent *a, float dt);
void agent_execute_chop(Agent *a);   // adds 1..g_chop_yield wood
void agent_execute_build(Agent *a);  // spends WOOD_PER_CHAIR wood, gains 1 chair

void agents_adjust_valuations(Agent *agents, int count, int numAgents, float delta, MarketId mid);
void agents_inject_money(Agent *agents, int count, int numAgents, float delta);
void agents_inject_goods(Agent *agents, int count, int numAgents, int delta, MarketId mid);

#endif
