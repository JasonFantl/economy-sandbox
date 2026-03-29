#ifndef MARKET_H
#define MARKET_H

#include "agent.h"

#define PRICE_HISTORY_SIZE  400

extern int g_allow_debt;  // 0 = normal (buyers need money), 1 = allow negative money

// Nerlove adaptive expectation: EMV_new = EMV_old^(1-alpha) * signal^alpha
#include <math.h>
static inline float nerlove_update(float emv, float signal, float alpha) {
    if (signal < 0.1f) signal = 0.1f;
    return powf(emv, 1.0f - alpha) * powf(signal, alpha);
}

// Per-agent value history (circular buffer)
typedef struct {
    float data[MAX_AGENTS][PRICE_HISTORY_SIZE];
    int   count;
    int   head;
    int   agentCount;
} AgentValueHistory;

// Gossip: agents nudge their EMVs for market mid toward each other on meeting.
void  market_gossip(Agent *a, Agent *b, MarketId mid);

// Trade: attempt a transaction for market mid between a buyer and a seller.
// Returns 1 if transaction occurred.
int   market_trade(Agent *buyer, Agent *seller, MarketId mid);

// Record expectedMarketValue for market mid
void  avh_record(AgentValueHistory *avh, const Agent *agents, int count, MarketId mid);
// Record market_potential_value() for market mid
void  avh_record_personal(AgentValueHistory *avh, const Agent *agents, int count, MarketId mid);
// Record goods count for market mid
void  avh_record_goods(AgentValueHistory *avh, const Agent *agents, int count, MarketId mid);

float avh_get(const AgentValueHistory *avh, int agentIdx, int sampleIdx);
float avh_avg(const AgentValueHistory *avh, int sampleIdx);

#endif
