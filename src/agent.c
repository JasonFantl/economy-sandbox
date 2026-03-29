#include "agent.h"
#include "market.h"
#include "assets.h"
#include "raylib.h"
#include <math.h>

float g_wood_break_prob  = 0.000f;  // wood doesn't break by default
float g_chair_break_prob = 0.003f;

void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth) {
    if (GetRandomValue(0, 1) == 0) {
        int t;
        do { t = GetRandomValue(0, agentCount - 1); } while (t == agent->id);
        agent->targetType = TARGET_AGENT;
        agent->targetId   = t;
    } else {
        agent->targetType = TARGET_POS;
        agent->targetX    = (float)GetRandomValue(20, (int)worldWidth - 20);
    }
}

// Decide one production action for a single agent and apply it immediately.
static void agent_decide_action(Agent *a) {
    AgentMarket *mw = AGENT_MKT(a, MARKET_WOOD);
    AgentMarket *mc = AGENT_MKT(a, MARKET_CHAIR);

    float leisureVal = leisure_value(&a->leisure);
    float upd        = utility_per_dollar(a->money);  // ₴ per $

    // Chop value: best of personal marginal utility or utility of selling at EMV
    float chopVal = market_potential_value(mw);
    float chopSellUtil = mw->expectedMarketValue * upd;
    if (chopSellUtil > chopVal) chopVal = chopSellUtil;

    // Build value: utility gain from new chair minus utility cost of WOOD_PER_CHAIR woods
    float buildVal = -999.0f;
    if (mw->goods >= WOOD_PER_CHAIR) {
        float chairGain = market_potential_value(mc);
        float chairSellUtil = mc->expectedMarketValue * upd;
        if (chairSellUtil > chairGain) chairGain = chairSellUtil;
        float woodCost  = market_current_value(mw) * (float)WOOD_PER_CHAIR;
        buildVal = chairGain - woodCost;
    }

    AgentAction best = ACTION_LEISURE;
    float bestVal    = leisureVal;
    if (chopVal  > bestVal) { best = ACTION_CHOP;  bestVal = chopVal;  }
    if (buildVal > bestVal) { best = ACTION_BUILD; }

    a->lastAction = best;
    if (best == ACTION_CHOP) {
        mw->goods++;
        a->leisure.idleTime = 0.0f;
    } else if (best == ACTION_BUILD) {
        mw->goods -= WOOD_PER_CHAIR;
        mc->goods++;
        a->leisure.idleTime = 0.0f;
    } else {
        a->leisure.idleTime += a->productionInterval;
    }
}

void agents_init(Agent *agents, int count, float worldWidth) {
    for (int i = 0; i < count; i++) {
        agents[i].id    = i;
        agents[i].x     = (float)GetRandomValue(20, (int)worldWidth - 20);
        agents[i].money = (float)GetRandomValue(300, 700);

        // Wood market
        AgentMarket *mw             = &agents[i].markets[MARKET_WOOD];
        mw->goods                   = GetRandomValue(8, 16);
        mw->basePersonalValue       = (float)GetRandomValue(20, 50);
        mw->minValue                = 0.0f;
        mw->halfValueAt             = 16.0f;
        mw->timeSinceLastTrade      = 0.0f;
        mw->maxTimeSinceLastTrade   = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        mw->expectedMarketValue     = market_potential_value(mw);

        // Chair market
        AgentMarket *mc             = &agents[i].markets[MARKET_CHAIR];
        mc->goods                   = GetRandomValue(0, 2);
        mc->basePersonalValue       = (float)GetRandomValue(60, 130);
        mc->minValue                = 0.0f;
        mc->halfValueAt             = 4.0f;
        mc->timeSinceLastTrade      = 0.0f;
        mc->maxTimeSinceLastTrade   = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        mc->expectedMarketValue     = market_potential_value(mc);

        // Leisure
        agents[i].leisure.basePersonalValue = (float)GetRandomValue(25, 45);
        agents[i].leisure.minValue          = (float)GetRandomValue(3, 10);
        agents[i].leisure.halfValueAt       = 3.0f;
        agents[i].leisure.idleTime          = 0.0f;

        agents[i].lastAction         = ACTION_LEISURE;
        // alpha: belief update speed — how quickly the agent incorporates new price info
        agents[i].alpha              = 0.05f + (float)GetRandomValue(0, 30) * 0.01f;
        agents[i].productionInterval = 1.0f + (float)GetRandomValue(0, 20) * 0.1f;
        agents[i].actionTimer        = agents[i].productionInterval;

        // Animation
        agents[i].spriteType  = GetRandomValue(0, SPRITE_TYPE_COUNT - 1);
        agents[i].animFrame   = GetRandomValue(0, SPRITE_WALK_FRAMES - 1);
        agents[i].animTimer   = 0.125f;
        agents[i].facingRight = 1;
        agents[i].tradeFlash  = 0.0f;

        agents_pick_new_target(&agents[i], count, worldWidth);
    }
}

void agents_update(Agent *agents, int count, float dt) {
    for (int i = 0; i < count; i++) {
        Agent *a = &agents[i];

        // Movement
        float tx = (a->targetType == TARGET_AGENT) ? agents[a->targetId].x : a->targetX;
        float dx = tx - a->x;
        if (fabsf(dx) > 1.0f) {
            float dir = (dx > 0.0f) ? 1.0f : -1.0f;
            a->x += dir * AGENT_SPEED * dt;
            a->facingRight = (dir > 0.0f) ? 1 : 0;
        }

        // Walk animation
        a->animTimer -= dt;
        if (a->animTimer <= 0.0f) {
            a->animFrame = (a->animFrame + 1) % SPRITE_WALK_FRAMES;
            a->animTimer = 0.125f;
        }

        if (a->tradeFlash > 0.0f) a->tradeFlash -= dt;

        // Wood breaking
        int woods = a->markets[MARKET_WOOD].goods;
        if (woods > 0 && g_wood_break_prob > 0.0f) {
            float breakP = g_wood_break_prob * dt * (float)woods;
            if ((float)GetRandomValue(0, 100000) * 0.00001f < breakP)
                a->markets[MARKET_WOOD].goods--;
        }

        // Chair breaking
        int chairs = a->markets[MARKET_CHAIR].goods;
        if (chairs > 0 && g_chair_break_prob > 0.0f) {
            float breakP = g_chair_break_prob * dt * (float)chairs;
            if ((float)GetRandomValue(0, 100000) * 0.00001f < breakP)
                a->markets[MARKET_CHAIR].goods--;
        }

        // Per-market idle frustration: when unable to trade, drift EMV toward
        // personal valuation using the Nerlove geometric update.
        // Buyers drift up toward their marginal value; sellers drift down toward their floor.
        float upd = utility_per_dollar(a->money);
        for (int mid = 0; mid < MARKET_COUNT; mid++) {
            AgentMarket *m = &a->markets[mid];
            int couldBuy  = market_is_buyer(m, a->money)  && (g_allow_debt || a->money >= m->expectedMarketValue);
            int couldSell = market_is_seller(m, a->money) && m->goods > 0;
            if (couldBuy || couldSell) {
                m->timeSinceLastTrade += dt;
                if (m->timeSinceLastTrade > m->maxTimeSinceLastTrade) {
                    m->timeSinceLastTrade = 0.0f;
                    // Convert personal utility to dollar indifference price: ₴ / (₴/$) = $
                    float utilVal = couldBuy ? market_potential_value(m) : market_current_value(m);
                    float signal  = utilVal / upd;
                    m->expectedMarketValue = nerlove_update(m->expectedMarketValue, signal, a->alpha);
                }
            }
        }

        // Production decision timer
        a->actionTimer -= dt;
        if (a->actionTimer <= 0.0f) {
            a->actionTimer = a->productionInterval;
            agent_decide_action(a);
        }
    }
}

// Fisher-Yates partial shuffle helper — reused by all three inject functions
static void partial_shuffle(int *indices, int count, int n) {
    for (int i = 0; i < n; i++) {
        int j   = i + GetRandomValue(0, count - 1 - i);
        int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
    }
}

void agents_influence(Agent *agents, int count, int n, float delta, MarketId mid) {
    if (n > count) n = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, n);
    for (int i = 0; i < n; i++) {
        float *pv = &agents[indices[i]].markets[mid].basePersonalValue;
        *pv += delta;
        if (*pv < 1.0f) *pv = 1.0f;
    }
}

void agents_give_money(Agent *agents, int count, int n, float delta) {
    if (n > count) n = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, n);
    for (int i = 0; i < n; i++)
        agents[indices[i]].money += delta;
}

void agents_give_goods(Agent *agents, int count, int n, int delta, MarketId mid) {
    if (n > count) n = count;
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    partial_shuffle(indices, count, n);
    for (int i = 0; i < n; i++) {
        int *goods = &agents[indices[i]].markets[mid].goods;
        *goods += delta;
        if (*goods < 0) *goods = 0;
    }
}
