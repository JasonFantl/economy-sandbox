#include "market.h"

void market_gossip(Agent *a, Agent *b) {
    // Each agent nudges their EMV toward the other's (snapshot before mutating)
    float aEMV = a->expectedMarketValue;
    float bEMV = b->expectedMarketValue;

    if (bEMV > aEMV) a->expectedMarketValue += BELIEF_VOLATILITY;
    else if (bEMV < aEMV) a->expectedMarketValue -= BELIEF_VOLATILITY;

    if (aEMV > bEMV) b->expectedMarketValue += BELIEF_VOLATILITY;
    else if (aEMV < bEMV) b->expectedMarketValue -= BELIEF_VOLATILITY;
}

int market_trade(Agent *buyer, Agent *seller) {
    if (buyer->expectedMarketValue >= seller->expectedMarketValue) {
        // Successful transaction: each seeks a better deal next time
        buyer->expectedMarketValue  -= BELIEF_VOLATILITY;
        seller->expectedMarketValue += BELIEF_VOLATILITY;
        buyer->timeSinceLastTrade    = 0.0f;
        seller->timeSinceLastTrade   = 0.0f;
        buyer->tradeFlash  = 0.4f;
        seller->tradeFlash = 0.4f;
        return 1;
    }
    // Prices incompatible: no immediate adjustment.
    // The idle timer in agents_update will handle it.
    return 0;
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
