#ifndef MARKET_H
#define MARKET_H

#include "econ/econ.h"

#define PRICE_HISTORY_SIZE  400

extern bool g_disable_executing_trade;  // true = price beliefs update but money/goods are not exchanged

// Nerlove adaptive expectation: price_new = price_old^(1-rate) * signal^rate
#include <math.h>
#include <stdbool.h>
static inline float nerlove_update(float price, float signal, float rate) {
    if (signal < 0.1f) signal = 0.1f;
    return powf(price, 1.0f - rate) * powf(signal, rate);
}

// Per-agent price-belief history (circular buffer, one row per agent)
typedef struct {
    float data[MAX_AGENTS][PRICE_HISTORY_SIZE];
    int   count;
    int   head;
    int   agentCount;
} AgentValueHistory;

// True if the agent wants to buy at a specific price (utility exceeds cost)
static inline int wants_to_buy(const Agent *a, float price, MarketId mid) {
    const AgentMarket *m = AGENT_MKT(a, mid);
    return marginal_buy_utility(m) > price * money_marginal_utility(a->econ.money);
}

// True if the agent wants to sell at a specific price (revenue exceeds utility of keeping)
static inline int wants_to_sell(const Agent *a, float price, MarketId mid) {
    const AgentMarket *m = AGENT_MKT(a, mid);
    return marginal_sell_utility(m) < price * money_marginal_utility(a->econ.money);
}

// True if the agent can afford to buy at this price
static inline int able_to_buy(const Agent *a, float price, MarketId mid) {
    (void)mid;
    return a->econ.money >= price;
}

// True if the agent has goods to sell
static inline int able_to_sell(const Agent *a, MarketId mid) {
    return AGENT_MKT(a, mid)->goods > 0;
}

// True if agent wants to buy/sell at their own expected price (role determination)
static inline int is_buyer(const Agent *a, MarketId mid) {
    return wants_to_buy(a, AGENT_MKT(a, mid)->priceExpectation, mid);
}
static inline int is_seller(const Agent *a, MarketId mid) {
    return able_to_sell(a, mid) && wants_to_sell(a, AGENT_MKT(a, mid)->priceExpectation, mid);
}

// Price belief exchange: each agent nudges their price expectation toward the other's
void market_gossip(Agent *a, Agent *b, MarketId mid);

// Attempt a trade between two agents for market mid.
// Determines buyer/seller roles internally; returns 1 if a transaction occurred.
int  market_trade(Agent *a, Agent *b, MarketId mid);

// Nudge a's price expectation toward their indifference price at the given rate.
void market_frustration_nudge(Agent *a, MarketId mid, float rate);

// History recording
void avh_record_prices(AgentValueHistory *h, const Agent *agents, int count, MarketId mid);
void avh_record_personal_valuations(AgentValueHistory *h, const Agent *agents, int count, MarketId mid);
void avh_record_goods(AgentValueHistory *h, const Agent *agents, int count, MarketId mid);
void avh_record_money(AgentValueHistory *h, const Agent *agents, int count);

float avh_get(const AgentValueHistory *h, int agentIdx, int sampleIdx);
float avh_avg(const AgentValueHistory *h, int sampleIdx);

#endif
