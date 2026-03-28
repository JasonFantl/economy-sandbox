#ifndef MARKET_H
#define MARKET_H

#include "agent.h"

#define PRICE_HISTORY_SIZE  400

// Per-agent expected market value history (circular buffer)
typedef struct {
    float data[MAX_AGENTS][PRICE_HISTORY_SIZE];
    int   count;
    int   head;
    int   agentCount;
} AgentValueHistory;

// Gossip: agents nudge their EMVs toward each other on meeting.
// Called on every agent encounter, regardless of buyer/seller status.
void market_gossip(Agent *a, Agent *b, float belief_factor);

// Trade: attempt a transaction between a buyer and a seller.
// Caller is responsible for ensuring a is buyer, b is seller.
// Resets idle timers on success. Returns 1 if transaction occurred.
int market_trade(Agent *buyer, Agent *seller);

void  avh_record(AgentValueHistory *avh, const Agent *agents, int count);
float avh_get(const AgentValueHistory *avh, int agentIdx, int sampleIdx);
float avh_avg(const AgentValueHistory *avh, int sampleIdx);

#endif
