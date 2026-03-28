#include "market.h"

void market_gossip(Agent *a, Agent *b, float belief_factor) {
    // Each agent nudges their EMV toward the other's (snapshot before mutating)
    float aEMV = a->expectedMarketValue;
    float bEMV = b->expectedMarketValue;

    if (bEMV > aEMV) a->expectedMarketValue += BELIEF_VOLATILITY * belief_factor;
    else if (bEMV < aEMV) a->expectedMarketValue -= BELIEF_VOLATILITY * belief_factor;

    if (aEMV > bEMV) b->expectedMarketValue += BELIEF_VOLATILITY * belief_factor;
    else if (aEMV < bEMV) b->expectedMarketValue -= BELIEF_VOLATILITY * belief_factor;
}

int market_trade(Agent *buyer, Agent *seller) {
    float price = seller->expectedMarketValue;

    // Check feasibility: seller has goods, buyer can afford and is willing
    if (seller->goods <= 0) return 0;
    if (buyer->money < price) return 0;
    if (buyer->expectedMarketValue < price) return 0;

    // Transfer goods and money at seller's asking price
    buyer->money  -= price;
    seller->money += price;
    buyer->goods++;
    seller->goods--;

    // Update beliefs: each party seeks a better deal next time
    buyer->expectedMarketValue  -= BELIEF_VOLATILITY;
    seller->expectedMarketValue += BELIEF_VOLATILITY;
    if (buyer->expectedMarketValue < 0.1f) buyer->expectedMarketValue = 0.1f;

    buyer->timeSinceLastTrade  = 0.0f;
    seller->timeSinceLastTrade = 0.0f;
    buyer->tradeFlash  = 0.4f;
    seller->tradeFlash = 0.4f;
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
