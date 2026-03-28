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
        // Same type: unmatched — nudge beliefs to encourage future trades
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
        // Successful: each pushes toward a better deal next time
        buyer->expectedMarketValue  -= BELIEF_VOLATILITY;
        seller->expectedMarketValue += BELIEF_VOLATILITY;
    } else {
        // Failed: each makes a better offer next time
        buyer->expectedMarketValue  += BELIEF_VOLATILITY;
        seller->expectedMarketValue -= BELIEF_VOLATILITY;
    }

    a->tradeFlash = 0.4f;
    b->tradeFlash = 0.4f;
    return 1;
}

void avh_record(AgentValueHistory *avh, const Agent *agents, int count) {
    avh->agentCount = count;
    for (int i = 0; i < count; i++) {
        avh->data[i][avh->head] = agents[i].expectedMarketValue;
    }
    avh->head = (avh->head + 1) % PRICE_HISTORY_SIZE;
    if (avh->count < PRICE_HISTORY_SIZE) avh->count++;
}

float avh_get(const AgentValueHistory *avh, int agentIdx, int sampleIdx) {
    int oldest = (avh->count < PRICE_HISTORY_SIZE) ? 0 : avh->head;
    int idx    = (oldest + sampleIdx) % PRICE_HISTORY_SIZE;
    return avh->data[agentIdx][idx];
}

float avh_avg(const AgentValueHistory *avh, int sampleIdx) {
    float sum = 0.0f;
    for (int i = 0; i < avh->agentCount; i++) {
        sum += avh_get(avh, i, sampleIdx);
    }
    return (avh->agentCount > 0) ? sum / (float)avh->agentCount : 0.0f;
}
