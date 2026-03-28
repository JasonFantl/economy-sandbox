#include "agent.h"
#include "assets.h"
#include "raylib.h"
#include <math.h>

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

void agents_init(Agent *agents, int count, float worldWidth) {
    for (int i = 0; i < count; i++) {
        agents[i].id                    = i;
        agents[i].x                     = (float)GetRandomValue(20, (int)worldWidth - 20);
        agents[i].money                 = (float)GetRandomValue(50, 150);
        agents[i].goods                 = GetRandomValue(0, 10);
        agents[i].basePersonalValue     = (float)GetRandomValue(20, 80);
        agents[i].halfValueAt           = 15.0f;
        agents[i].timeSinceLastTrade    = 0.0f;
        agents[i].maxTimeSinceLastTrade = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        agents[i].tradeFlash            = 0.0f;
        agents[i].spriteType            = GetRandomValue(0, SPRITE_TYPE_COUNT - 1);
        agents[i].animFrame             = GetRandomValue(0, SPRITE_WALK_FRAMES - 1);
        agents[i].animTimer             = 0.125f;
        agents[i].facingRight           = 1;
        // Start with a price belief matching their current marginal value
        agents[i].expectedMarketValue   = agent_potential_value(&agents[i]);
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
            a->animTimer = 0.125f; // 8 fps
        }

        if (a->tradeFlash > 0.0f) a->tradeFlash -= dt;

        // Idle timer: only ticks when the agent could trade but hasn't.
        // Buyer must have enough money; seller must have goods.
        int couldBuy  = agent_is_buyer(a)  && a->money >= a->expectedMarketValue;
        int couldSell = agent_is_seller(a);
        if (couldBuy || couldSell) {
            a->timeSinceLastTrade += dt;
            if (a->timeSinceLastTrade > a->maxTimeSinceLastTrade) {
                a->timeSinceLastTrade = 0.0f;
                // Adjust price to become more attractive to the market
                if (couldBuy)  a->expectedMarketValue += BELIEF_VOLATILITY;
                else           a->expectedMarketValue -= BELIEF_VOLATILITY;
                if (a->expectedMarketValue < 0.1f) a->expectedMarketValue = 0.1f;
            }
        }
    }
}

void agents_influence(Agent *agents, int count, int n, float delta) {
    if (n > count) n = count;
    // Partial Fisher-Yates: pick n distinct agents without replacement
    int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    for (int i = 0; i < n; i++) {
        int j   = i + GetRandomValue(0, count - 1 - i);
        int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
        float *pv = &agents[indices[i]].basePersonalValue;
        *pv += delta;
        if (*pv < 1.0f) *pv = 1.0f;
    }
}
