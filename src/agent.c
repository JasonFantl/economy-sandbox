#include "agent.h"
#include "raylib.h"
#include <math.h>

void agents_init(Agent *agents, int count, float worldWidth) {
    for (int i = 0; i < count; i++) {
        agents[i].id                 = i;
        agents[i].x                  = (float)GetRandomValue(20, (int)worldWidth - 20);
        agents[i].personalValue      = (float)GetRandomValue(20, 80);
        agents[i].expectedMarketValue = (float)GetRandomValue(20, 80);
        agents[i].targetId           = agents_pick_target(count, i);
        agents[i].tradeFlash         = 0.0f;
    }
}

void agents_update(Agent *agents, int count, float dt) {
    for (int i = 0; i < count; i++) {
        Agent *a      = &agents[i];
        Agent *target = &agents[a->targetId];

        float dx   = target->x - a->x;
        float dist = fabsf(dx);
        if (dist > 1.0f) {
            float dir = (dx > 0.0f) ? 1.0f : -1.0f;
            a->x += dir * AGENT_SPEED * dt;
        }

        if (a->tradeFlash > 0.0f) {
            a->tradeFlash -= dt;
        }
    }
}

int agents_pick_target(int count, int selfId) {
    int t;
    do {
        t = GetRandomValue(0, count - 1);
    } while (t == selfId);
    return t;
}
