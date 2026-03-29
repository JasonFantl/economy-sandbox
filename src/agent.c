#include "agent.h"
#include "assets.h"
#include "raylib.h"
#include <math.h>

float g_wood_break_prob  = 0.000f;
float g_chair_break_prob = 0.003f;

// Declared in econ.c; called once per agent per frame from agents_update.
void econ_update(Agent *a, float dt);

void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth) {
    if (GetRandomValue(0, 1) == 0) {
        int t;
        do { t = GetRandomValue(0, agentCount - 1); } while (t == agent->body.id);
        agent->body.targetType = TARGET_AGENT;
        agent->body.targetId   = t;
    } else {
        agent->body.targetType = TARGET_POS;
        agent->body.targetX    = (float)GetRandomValue(20, (int)worldWidth - 20);
    }
}

void agents_init(Agent *agents, int count, float worldWidth) {
    for (int i = 0; i < count; i++) {
        // --- Body ---
        agents[i].body.id         = i;
        agents[i].body.x          = (float)GetRandomValue(20, (int)worldWidth - 20);

        // --- Econ ---
        agents[i].econ.money = (float)GetRandomValue(300, 700);

        AgentMarket *mw             = &agents[i].econ.markets[MARKET_WOOD];
        mw->goods                   = GetRandomValue(8, 16);
        mw->basePersonalValue       = (float)GetRandomValue(20, 50);
        mw->minValue                = 0.0f;
        mw->halfValueAt             = 16.0f;
        mw->timeSinceLastTrade      = 0.0f;
        mw->maxTimeSinceLastTrade   = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        mw->expectedMarketValue     = market_potential_value(mw);

        AgentMarket *mc             = &agents[i].econ.markets[MARKET_CHAIR];
        mc->goods                   = GetRandomValue(0, 2);
        mc->basePersonalValue       = (float)GetRandomValue(60, 130);
        mc->minValue                = 0.0f;
        mc->halfValueAt             = 4.0f;
        mc->timeSinceLastTrade      = 0.0f;
        mc->maxTimeSinceLastTrade   = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        mc->expectedMarketValue     = market_potential_value(mc);

        agents[i].econ.leisure.basePersonalValue = (float)GetRandomValue(25, 45);
        agents[i].econ.leisure.minValue          = (float)GetRandomValue(3, 10);
        agents[i].econ.leisure.halfValueAt       = 3.0f;
        agents[i].econ.leisure.idleTime          = 0.0f;

        agents[i].econ.lastAction         = ACTION_LEISURE;
        agents[i].econ.alpha              = 0.05f + (float)GetRandomValue(0, 30) * 0.01f;
        agents[i].econ.productionInterval = 1.0f + (float)GetRandomValue(0, 20) * 0.1f;
        agents[i].econ.actionTimer        = agents[i].econ.productionInterval;

        // --- Sprite ---
        agents[i].sprite.spriteType  = GetRandomValue(0, SPRITE_TYPE_COUNT - 1);
        agents[i].sprite.animFrame   = GetRandomValue(0, SPRITE_WALK_FRAMES - 1);
        agents[i].sprite.animTimer   = 0.125f;
        agents[i].sprite.facingRight = 1;
        agents[i].sprite.tradeFlash  = 0.0f;

        agents_pick_new_target(&agents[i], count, worldWidth);
    }
}

void agents_update(Agent *agents, int count, float dt) {
    for (int i = 0; i < count; i++) {
        Agent *a = &agents[i];

        // --- Body: movement ---
        float tx = (a->body.targetType == TARGET_AGENT)
                   ? agents[a->body.targetId].body.x
                   : a->body.targetX;
        float dx = tx - a->body.x;
        if (fabsf(dx) > 1.0f) {
            float dir = (dx > 0.0f) ? 1.0f : -1.0f;
            a->body.x           += dir * AGENT_SPEED * dt;
            a->sprite.facingRight = (dir > 0.0f) ? 1 : 0;
        }

        // --- Sprite: walk animation ---
        a->sprite.animTimer -= dt;
        if (a->sprite.animTimer <= 0.0f) {
            a->sprite.animFrame = (a->sprite.animFrame + 1) % SPRITE_WALK_FRAMES;
            a->sprite.animTimer = 0.125f;
        }
        if (a->sprite.tradeFlash > 0.0f)
            a->sprite.tradeFlash -= dt;

        // --- Econ: goods decay and economic decisions ---
        econ_update(a, dt);
    }
}
