#include "agent.h"
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
        agents[i].id                   = i;
        agents[i].x                    = (float)GetRandomValue(20, (int)worldWidth - 20);
        agents[i].personalValue        = (float)GetRandomValue(20, 80);
        agents[i].expectedMarketValue  = (float)GetRandomValue(20, 80);
        agents[i].tradeFlash           = 0.0f;
        agents[i].timeSinceLastTrade   = 0.0f;
        agents[i].maxTimeSinceLastTrade = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
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
            a->x += (dx > 0.0f ? 1.0f : -1.0f) * AGENT_SPEED * dt;
        }

        if (a->tradeFlash > 0.0f) a->tradeFlash -= dt;

        // Idle timer: only ticks when agent could trade but hasn't
        int isBuyer  = (a->personalValue > a->expectedMarketValue);
        int isSeller = (a->personalValue < a->expectedMarketValue);
        if (isBuyer || isSeller) {
            a->timeSinceLastTrade += dt;
            if (a->timeSinceLastTrade > a->maxTimeSinceLastTrade) {
                a->timeSinceLastTrade = 0.0f;
                // Become more attractive to the market
                if (isBuyer) a->expectedMarketValue += BELIEF_VOLATILITY;
                else         a->expectedMarketValue -= BELIEF_VOLATILITY;
            }
        }
    }
}
