#include "agent.h"
#include "assets.h"
#include "raylib.h"
#include <math.h>

float g_wood_decay_rate  = 0.000f;
float g_chair_decay_rate = 0.003f;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void agents_init(Agent *agents, int count, float worldWidth, float worldHeight) {
    for (int i = 0; i < count; i++) {
        agents[i].body.id = i;

        float nx, ny;
        if (nav_random_position(&nx, &ny)) {
            agents[i].body.x = nx;
            agents[i].body.y = ny;
        } else {
            agents[i].body.x = (float)GetRandomValue(20, (int)worldWidth  - 20);
            agents[i].body.y = (float)GetRandomValue(20, (int)worldHeight - 20);
        }
        agents[i].body.wpCount = 0;
        agents[i].body.wpIdx   = 0;

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

        agents[i].econ.leisure.maxUtility     = (float)GetRandomValue(25, 45);
        agents[i].econ.leisure.minUtility     = (float)GetRandomValue(3, 10);
        agents[i].econ.leisure.halfSaturation = 3.0f;
        agents[i].econ.leisure.idleTime       = 0.0f;

        agents[i].econ.lastAction       = ACTION_LEISURE;
        agents[i].econ.pendingWork      = ACTION_LEISURE;
        agents[i].econ.beliefUpdateRate = 0.05f + (float)GetRandomValue(0, 30) * 0.01f;
        agents[i].econ.productionPeriod = 1.0f + (float)GetRandomValue(0, 20) * 0.1f;
        agents[i].econ.productionTimer  = agents[i].econ.productionPeriod;

        agents[i].sprite.spriteType = GetRandomValue(0, WORKER_TYPE_COUNT - 1);
        agents[i].sprite.animFrame  = GetRandomValue(0, WORKER_WALK_FRAMES - 1);
        agents[i].sprite.animTimer  = 0.15f;
        agents[i].sprite.facing     = DIR_SOUTH;
        agents[i].sprite.tradeFlash = 0.0f;

        agents_pick_new_target(agents, i, count, worldWidth, worldHeight);
    }
}

// ---------------------------------------------------------------------------
// Update — O(1) per agent per step
// ---------------------------------------------------------------------------

#define WAYPOINT_RADIUS 10.0f  // must match nav.c

void agents_update(Agent *agents, int count, float dt) {
    for (int i = 0; i < count; i++) {
        Agent *a = &agents[i];

        // Follow precomputed waypoints (both TARGET_AGENT and TARGET_POS)
        float tx, ty;
        if (a->body.wpIdx < a->body.wpCount) {
            tx = a->body.wpPx[a->body.wpIdx];
            ty = a->body.wpPy[a->body.wpIdx];
            float wdx = tx - a->body.x, wdy = ty - a->body.y;
            if (wdx*wdx + wdy*wdy <= WAYPOINT_RADIUS * WAYPOINT_RADIUS) {
                a->body.wpIdx++;
                if (a->body.wpIdx < a->body.wpCount) {
                    tx = a->body.wpPx[a->body.wpIdx];
                    ty = a->body.wpPy[a->body.wpIdx];
                } else {
                    tx = a->body.targetX;
                    ty = a->body.targetY;
                }
            }
        } else {
            tx = a->body.targetX;
            ty = a->body.targetY;
        }

        float dx = tx - a->body.x, dy = ty - a->body.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 1.0f) {
            float inv = 1.0f / dist;
            a->body.x += dx * inv * AGENT_SPEED * dt;
            a->body.y += dy * inv * AGENT_SPEED * dt;
            if (fabsf(dx) >= fabsf(dy))
                a->sprite.facing = (dx > 0.0f) ? DIR_EAST : DIR_WEST;
            else
                a->sprite.facing = (dy > 0.0f) ? DIR_SOUTH : DIR_NORTH;
        }

        a->sprite.animTimer -= dt;
        if (a->sprite.animTimer <= 0.0f) {
            a->sprite.animFrame = (a->sprite.animFrame + 1) % WORKER_WALK_FRAMES;
            a->sprite.animTimer = 0.15f;
        }
        if (a->sprite.tradeFlash > 0.0f) a->sprite.tradeFlash -= dt;

        agent_econ_update(a, dt);
    }
}
