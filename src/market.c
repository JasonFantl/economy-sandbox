#include "market.h"

int g_allow_debt = 0;

void market_gossip(Agent *a, Agent *b, MarketId mid) {
    AgentMarket *ma = AGENT_MKT(a, mid);
    AgentMarket *mb = AGENT_MKT(b, mid);
    // Snapshot both before updating either
    float aEMV = ma->expectedMarketValue;
    float bEMV = mb->expectedMarketValue;
    // Each agent updates toward the other's EMV using their own alpha
    ma->expectedMarketValue = nerlove_update(aEMV, bEMV, a->econ.alpha);
    mb->expectedMarketValue = nerlove_update(bEMV, aEMV, b->econ.alpha);
}

int market_trade(Agent *buyer, Agent *seller, MarketId mid) {
    AgentMarket *mb = AGENT_MKT(buyer,  mid);
    AgentMarket *ms = AGENT_MKT(seller, mid);
    float price = ms->expectedMarketValue;

    if (ms->goods <= 0) return 0;
    if (!g_allow_debt && buyer->econ.money < price) return 0;
    if (mb->expectedMarketValue < price) return 0;

    buyer->econ.money  -= price;
    seller->econ.money += price;
    mb->goods++;
    ms->goods--;

    // Both agents update their EMV toward the actual trade price
    mb->expectedMarketValue = nerlove_update(mb->expectedMarketValue, price, buyer->econ.alpha);
    ms->expectedMarketValue = nerlove_update(ms->expectedMarketValue, price, seller->econ.alpha);

    mb->timeSinceLastTrade = 0.0f;
    ms->timeSinceLastTrade = 0.0f;
    buyer->sprite.tradeFlash  = 0.4f;
    seller->sprite.tradeFlash = 0.4f;
    return 1;
}

void avh_record(AgentValueHistory *avh, const Agent *agents, int count, MarketId mid) {
    avh->agentCount = count;
    for (int i = 0; i < count; i++)
        avh->data[i][avh->head] = agents[i].econ.markets[mid].expectedMarketValue;
    avh->head = (avh->head + 1) % PRICE_HISTORY_SIZE;
    if (avh->count < PRICE_HISTORY_SIZE) avh->count++;
}

void avh_record_personal(AgentValueHistory *avh, const Agent *agents, int count, MarketId mid) {
    avh->agentCount = count;
    for (int i = 0; i < count; i++)
        avh->data[i][avh->head] = market_potential_value(&agents[i].econ.markets[mid]);
    avh->head = (avh->head + 1) % PRICE_HISTORY_SIZE;
    if (avh->count < PRICE_HISTORY_SIZE) avh->count++;
}

void avh_record_goods(AgentValueHistory *avh, const Agent *agents, int count, MarketId mid) {
    avh->agentCount = count;
    for (int i = 0; i < count; i++)
        avh->data[i][avh->head] = (float)agents[i].econ.markets[mid].goods;
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
    for (int i = 0; i < avh->agentCount; i++)
        sum += avh_get(avh, i, sampleIdx);
    return (avh->agentCount > 0) ? sum / (float)avh->agentCount : 0.0f;
}
