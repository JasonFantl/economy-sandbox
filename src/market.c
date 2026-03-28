#include "market.h"
#include <stddef.h>

int market_trade(Agent *a, Agent *b) {
    int aIsBuyer = (a->personalValue > a->expectedMarketValue);
    int bIsBuyer = (b->personalValue > b->expectedMarketValue);

    Agent *buyer  = NULL;
    Agent *seller = NULL;

    if (aIsBuyer && !bIsBuyer) {
        buyer = a; seller = b;
    } else if (!aIsBuyer && bIsBuyer) {
        buyer = b; seller = a;
    } else {
        // Both same type: unmatched — adjust beliefs to encourage future trades
        if (aIsBuyer) {
            a->expectedMarketValue += BELIEF_VOLATILITY;
            b->expectedMarketValue += BELIEF_VOLATILITY;
        } else {
            a->expectedMarketValue -= BELIEF_VOLATILITY;
            b->expectedMarketValue -= BELIEF_VOLATILITY;
        }
        a->tradeFlash = 0.2f;
        b->tradeFlash = 0.2f;
        return 0;
    }

    if (buyer->expectedMarketValue >= seller->expectedMarketValue) {
        // Successful transaction: push toward better deal next time
        buyer->expectedMarketValue  -= BELIEF_VOLATILITY;
        seller->expectedMarketValue += BELIEF_VOLATILITY;
    } else {
        // Failed transaction: make better offers next time
        buyer->expectedMarketValue  += BELIEF_VOLATILITY;
        seller->expectedMarketValue -= BELIEF_VOLATILITY;
    }

    a->tradeFlash = 0.4f;
    b->tradeFlash = 0.4f;
    return 1;
}

void price_history_record(PriceHistory *ph, const Agent *agents, int count) {
    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        sum += agents[i].expectedMarketValue;
    }
    float avg = sum / (float)count;

    ph->values[ph->head] = avg;
    ph->head = (ph->head + 1) % PRICE_HISTORY_SIZE;
    if (ph->count < PRICE_HISTORY_SIZE) {
        ph->count++;
    }
}

float price_history_get(const PriceHistory *ph, int i) {
    // index 0 = oldest, count-1 = newest
    int oldest = (ph->count < PRICE_HISTORY_SIZE)
                 ? 0
                 : ph->head; // head points to oldest when full
    int idx = (oldest + i) % PRICE_HISTORY_SIZE;
    return ph->values[idx];
}
