#include "econ/market.h"
#include <stddef.h>
#include <stdbool.h>

bool g_disable_executing_trade = false;

void market_gossip(Agent *a, Agent *b, MarketId mid) {
    AgentMarket *ma = AGENT_MKT(a, mid);
    AgentMarket *mb = AGENT_MKT(b, mid);
    // Snapshot both before updating either
    Price a_price = ma->priceExpectation;
    Price b_price = mb->priceExpectation;
    // Each agent updates toward the other's price expectation using their own belief update rate
    ma->priceExpectation = nerlove_update(a_price, b_price, a->econ.beliefUpdateRate);
    mb->priceExpectation = nerlove_update(b_price, a_price, b->econ.beliefUpdateRate);
}

int market_trade(Agent *a, Agent *b, MarketId mid) {
    Agent *buyer = a;
    Agent *seller = b;                                                                                                                                                                               
    if      (is_buyer(a, mid) && is_seller(b, mid)) { buyer = a; seller = b; }                                                                                                                    
    else if (is_buyer(b, mid) && is_seller(a, mid)) { buyer = b; seller = a; }                                                                                                                    
    else {
        // // small amount of frustration if agents can't pair into buyers/sellers
        market_frustration_nudge(a, mid, 0.02f);
        market_frustration_nudge(b, mid, 0.02f);
        return 0;
    }

    AgentMarket *buyer_mkt  = AGENT_MKT(buyer,  mid);
    AgentMarket *seller_mkt = AGENT_MKT(seller, mid);
    Price proposed_price = seller_mkt->priceExpectation; // assumes sellers are the ones to publicize price

    if (!able_to_buy(buyer, proposed_price, mid) || !wants_to_buy(buyer, proposed_price, mid)) {
        // frustration if buyer/seller do not agree on a fair price
        market_frustration_nudge(buyer, mid, buyer->econ.beliefUpdateRate);
        market_frustration_nudge(seller, mid, seller->econ.beliefUpdateRate);
        return 0;
    }

    if (!g_disable_executing_trade) {
        buyer->econ.money  -= proposed_price;
        seller->econ.money += proposed_price;
        buyer_mkt->goods++;
        seller_mkt->goods--;
    }

    // nudge prices to try and get a better deal next time
    buyer_mkt->priceExpectation  = nerlove_update(buyer_mkt->priceExpectation,  proposed_price, buyer->econ.beliefUpdateRate);
    seller_mkt->priceExpectation = nerlove_update(seller_mkt->priceExpectation, proposed_price, seller->econ.beliefUpdateRate);

    buyer->sprite.tradeFlashTick  = TRADE_FLASH_TICKS;
    seller->sprite.tradeFlashTick = TRADE_FLASH_TICKS;
    return 1;
}

void market_frustration_nudge(Agent *a, MarketId mid, float rate) {
    AgentMarket *m = AGENT_MKT(a, mid);
    // Only nudge if the agent wants to trade and is capable of doing so
    int frustrated = (is_buyer(a, mid) && able_to_buy(a, m->priceExpectation, mid))
                  || (is_seller(a, mid) && able_to_sell(a, mid));
    if (!frustrated) return;
    Utility money_utility    = money_marginal_utility(a->econ.money);
    Price indifference_price = (marginal_buy_utility(m) + marginal_sell_utility(m))
                               / (2.0f * money_utility);
    m->priceExpectation = nerlove_update(m->priceExpectation, indifference_price, rate);
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

void avh_record_money(AgentValueHistory *h, const Agent *agents, int count) {
    h->agentCount = count;
    for (int i = 0; i < count; i++)
        h->data[i][h->head] = agents[i].econ.money;
    h->head = (h->head + 1) % PRICE_HISTORY_SIZE;
    if (h->count < PRICE_HISTORY_SIZE) h->count++;
}

void speed_history_record(SpeedHistory *h, int speed) {
    h->data[h->head] = (uint8_t)(speed > 255 ? 255 : speed < 1 ? 1 : speed);
    h->head = (h->head + 1) % PRICE_HISTORY_SIZE;
    if (h->count < PRICE_HISTORY_SIZE) h->count++;
}

int speed_history_get(const SpeedHistory *h, int sampleIdx) {
    int oldest = (h->count < PRICE_HISTORY_SIZE) ? 0 : h->head;
    int idx    = (oldest + sampleIdx) % PRICE_HISTORY_SIZE;
    return (int)h->data[idx];
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
