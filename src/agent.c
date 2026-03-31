#include "agent.h"
#include "assets.h"
#include "raylib.h"
#include <math.h>

float g_wood_decay_rate  = 0.000f;
float g_chair_decay_rate = 0.003f;

// Declared in econ.c; called once per agent per frame from agents_update.
void agent_econ_update(Agent *a, float dt);

void agents_pick_new_target(Agent *agent, int agentCount,
                             float worldWidth, float worldHeight) {
    if (GetRandomValue(0, 1) == 0) {
        int t;
        do { t = GetRandomValue(0, agentCount - 1); } while (t == agent->body.id);
        agent->body.targetType = TARGET_AGENT;
        agent->body.targetId   = t;
    } else {
        agent->body.targetType = TARGET_POS;
        agent->body.targetX    = (float)GetRandomValue(20, (int)worldWidth  - 20);
        agent->body.targetY    = (float)GetRandomValue(20, (int)worldHeight - 20);
    }
}

void agents_init(Agent *agents, int count, float worldWidth, float worldHeight) {
    for (int i = 0; i < count; i++) {
        // --- Body ---
        agents[i].body.id = i;
        agents[i].body.x  = (float)GetRandomValue(20, (int)worldWidth  - 20);
        agents[i].body.y  = (float)GetRandomValue(20, (int)worldHeight - 20);

        // --- Econ ---
        agents[i].econ.money = (float)GetRandomValue(300, 700);

        AgentMarket *wood               = &agents[i].econ.markets[MARKET_WOOD];
        wood->goods                     = GetRandomValue(8, 16);
        wood->maxUtility                = (float)GetRandomValue(20, 50);
        wood->minUtility                = 0.0f;
        wood->halfSaturation            = 16.0f;
        wood->frustrationTimer          = 0.0f;
        wood->frustrationThreshold      = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        wood->priceExpectation          = marginal_buy_utility(wood);

        AgentMarket *chair              = &agents[i].econ.markets[MARKET_CHAIR];
        chair->goods                    = GetRandomValue(0, 2);
        chair->maxUtility               = (float)GetRandomValue(60, 130);
        chair->minUtility               = 0.0f;
        chair->halfSaturation           = 4.0f;
        chair->frustrationTimer         = 0.0f;
        chair->frustrationThreshold     = 5.0f + (float)GetRandomValue(0, 50) * 0.1f;
        chair->priceExpectation         = marginal_buy_utility(chair);

        agents[i].econ.leisure.maxUtility    = (float)GetRandomValue(25, 45);
        agents[i].econ.leisure.minUtility    = (float)GetRandomValue(3, 10);
        agents[i].econ.leisure.halfSaturation = 3.0f;
        agents[i].econ.leisure.idleTime      = 0.0f;

        agents[i].econ.lastAction       = ACTION_LEISURE;
        agents[i].econ.beliefUpdateRate = 0.05f + (float)GetRandomValue(0, 30) * 0.01f;
        agents[i].econ.productionPeriod = 1.0f + (float)GetRandomValue(0, 20) * 0.1f;
        agents[i].econ.productionTimer  = agents[i].econ.productionPeriod;

        // --- Sprite ---
        agents[i].sprite.spriteType = GetRandomValue(0, WORKER_TYPE_COUNT - 1);
        agents[i].sprite.animFrame  = GetRandomValue(0, WORKER_WALK_FRAMES - 1);
        agents[i].sprite.animTimer  = 0.15f;
        agents[i].sprite.facing     = DIR_SOUTH;
        agents[i].sprite.tradeFlash = 0.0f;

        agents_pick_new_target(&agents[i], count, worldWidth, worldHeight);
    }
}

void agents_update(Agent *agents, int count, float dt) {
    for (int i = 0; i < count; i++) {
        Agent *a = &agents[i];

        // --- Body: 2D movement ---
        float tx = (a->body.targetType == TARGET_AGENT)
                   ? agents[a->body.targetId].body.x : a->body.targetX;
        float ty = (a->body.targetType == TARGET_AGENT)
                   ? agents[a->body.targetId].body.y : a->body.targetY;

        float dx = tx - a->body.x;
        float dy = ty - a->body.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.0f) {
            float nx = dx / dist;   // normalised direction
            float ny = dy / dist;
            a->body.x += nx * AGENT_SPEED * dt;
            a->body.y += ny * AGENT_SPEED * dt;

            // Pick facing based on dominant movement axis
            if (fabsf(dx) >= fabsf(dy))
                a->sprite.facing = (dx > 0.0f) ? DIR_EAST : DIR_WEST;
            else
                a->sprite.facing = (dy > 0.0f) ? DIR_SOUTH : DIR_NORTH;
        }

        // --- Sprite: walk animation ---
        a->sprite.animTimer -= dt;
        if (a->sprite.animTimer <= 0.0f) {
            a->sprite.animFrame = (a->sprite.animFrame + 1) % WORKER_WALK_FRAMES;
            a->sprite.animTimer = 0.15f;
        }
        if (a->sprite.tradeFlash > 0.0f)
            a->sprite.tradeFlash -= dt;

        // --- Econ: goods decay and economic decisions ---
        agent_econ_update(a, dt);
    }
}
