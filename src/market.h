#ifndef MARKET_H
#define MARKET_H

#include "agent.h"

#define BELIEF_VOLATILITY   0.5f
#define PRICE_HISTORY_SIZE  400  // samples per agent; 400 * MAX_AGENTS * 4B = 320KB

// Per-agent expected market value history (circular buffer)
typedef struct {
    float data[MAX_AGENTS][PRICE_HISTORY_SIZE];
    int   count;       // samples recorded so far (up to PRICE_HISTORY_SIZE)
    int   head;        // next write index
    int   agentCount;
} AgentValueHistory;

// Attempt a trade when two agents meet. Returns 1 if a transaction occurred.
int market_trade(Agent *a, Agent *b);

void  avh_record(AgentValueHistory *avh, const Agent *agents, int count);
// i = 0 → oldest sample, i = avh->count-1 → newest
float avh_get(const AgentValueHistory *avh, int agentIdx, int i);
float avh_avg(const AgentValueHistory *avh, int sampleIdx);

#endif
