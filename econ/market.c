#include "econ/market.h"

int g_allow_debt = 0;

void market_gossip(Agent *a, Agent *b, MarketId mid) {
    AgentMarket *ma = AGENT_MKT(a, mid);
    AgentMarket *mb = AGENT_MKT(b, mid);
    // Snapshot both before updating either
    float a_price = ma->priceExpectation;
    float b_price = mb->priceExpectation;
    // Each agent updates toward the other's price expectation using their own belief update rate
    ma->priceExpectation = nerlove_update(a_price, b_price, a->econ.beliefUpdateRate);
    mb->priceExpectation = nerlove_update(b_price, a_price, b->econ.beliefUpdateRate);
}

int market_trade(Agent *buyer, Agent *seller, MarketId mid) {
    AgentMarket *buyer_mkt  = AGENT_MKT(buyer,  mid);
    AgentMarket *seller_mkt = AGENT_MKT(seller, mid);
    float price = seller_mkt->priceExpectation;

    if (seller_mkt->goods <= 0) return 0;
    if (!g_allow_debt && buyer->econ.money < price) return 0;
    if (buyer_mkt->priceExpectation < price) return 0;

    buyer->econ.money  -= price;
    seller->econ.money += price;
    buyer_mkt->goods++;
    seller_mkt->goods--;

    // Both agents update their price expectation toward the actual trade price
    buyer_mkt->priceExpectation  = nerlove_update(buyer_mkt->priceExpectation,  price, buyer->econ.beliefUpdateRate);
    seller_mkt->priceExpectation = nerlove_update(seller_mkt->priceExpectation, price, seller->econ.beliefUpdateRate);

    buyer_mkt->frustrationTick   = 0;
    seller_mkt->frustrationTick  = 0;
    buyer->sprite.tradeFlashTick  = TRADE_FLASH_TICKS;
    seller->sprite.tradeFlashTick = TRADE_FLASH_TICKS;
    return 1;
}

void avh_record_prices(AgentValueHistory *h, const Agent *agents, int count, MarketId mid) {
    h->agentCount = count;
    for (int i = 0; i < count; i++)
        h->data[i][h->head] = agents[i].econ.markets[mid].priceExpectation;
    h->head = (h->head + 1) % PRICE_HISTORY_SIZE;
    if (h->count < PRICE_HISTORY_SIZE) h->count++;
}

void avh_record_personal_valuations(AgentValueHistory *h, const Agent *agents, int count, MarketId mid) {
    h->agentCount = count;
    for (int i = 0; i < count; i++)
        h->data[i][h->head] = marginal_buy_utility(&agents[i].econ.markets[mid]);
    h->head = (h->head + 1) % PRICE_HISTORY_SIZE;
    if (h->count < PRICE_HISTORY_SIZE) h->count++;
}

void avh_record_goods(AgentValueHistory *h, const Agent *agents, int count, MarketId mid) {
    h->agentCount = count;
    for (int i = 0; i < count; i++)
        h->data[i][h->head] = (float)agents[i].econ.markets[mid].goods;
    h->head = (h->head + 1) % PRICE_HISTORY_SIZE;
    if (h->count < PRICE_HISTORY_SIZE) h->count++;
}

float avh_get(const AgentValueHistory *h, int agentIdx, int sampleIdx) {
    int oldest = (h->count < PRICE_HISTORY_SIZE) ? 0 : h->head;
    int idx    = (oldest + sampleIdx) % PRICE_HISTORY_SIZE;
    return h->data[agentIdx][idx];
}

float avh_avg(const AgentValueHistory *h, int sampleIdx) {
    float sum = 0.0f;
    for (int i = 0; i < h->agentCount; i++)
        sum += avh_get(h, i, sampleIdx);
    return (h->agentCount > 0) ? sum / (float)h->agentCount : 0.0f;
}
