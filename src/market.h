#ifndef MARKET_H
#define MARKET_H

#include "agent.h"

#define BELIEF_VOLATILITY    0.5f
#define PRICE_HISTORY_SIZE   600

typedef struct {
    float values[PRICE_HISTORY_SIZE]; // circular buffer of avg expectedMarketValue
    int   count;
    int   head;
} PriceHistory;

// Attempt a trade when two agents meet. Returns 1 if a transaction occurred.
int market_trade(Agent *a, Agent *b);

void price_history_record(PriceHistory *ph, const Agent *agents, int count);
// Get i-th oldest entry (0 = oldest, ph->count-1 = newest)
float price_history_get(const PriceHistory *ph, int i);

#endif
