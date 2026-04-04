#include "econ/agent.h"
#include "econ/econ.h"
#include "econ/market.h"
#include "econ/nav.h"
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

        AgentMarket *wood                  = &agents[i].econ.markets[MARKET_WOOD];
        wood->goods                        = GetRandomValue(8, 16);
        wood->maxUtility                   = (float)GetRandomValue(20, 50);
        wood->minUtility                   = 0.0f;
        wood->halfSaturation               = 16.0f;
        wood->frustrationTick              = 0;
        wood->frustrationThresholdTicks    = 300 + GetRandomValue(0, 50) * 6;
        wood->priceExpectation             = marginal_buy_utility(wood);

        AgentMarket *chair                 = &agents[i].econ.markets[MARKET_CHAIR];
        chair->goods                       = GetRandomValue(0, 2);
        chair->maxUtility                  = (float)GetRandomValue(60, 130);
        chair->minUtility                  = 0.0f;
        chair->halfSaturation              = 4.0f;
        chair->frustrationTick             = 0;
        chair->frustrationThresholdTicks   = 300 + GetRandomValue(0, 50) * 6;
        chair->priceExpectation            = marginal_buy_utility(chair);

        agents[i].econ.leisure.maxUtility        = (float)GetRandomValue(25, 45);
        agents[i].econ.leisure.minUtility        = (float)GetRandomValue(3, 10);
        agents[i].econ.leisure.halfSaturationTicks = 180; // 3.0s * 60
        agents[i].econ.leisure.idleTicks         = 0;

        agents[i].econ.lastAction            = ACTION_LEISURE;
        agents[i].econ.pendingWork           = ACTION_LEISURE;
        agents[i].econ.beliefUpdateRate      = 0.05f + (float)GetRandomValue(0, 30) * 0.01f;
        agents[i].econ.productionPeriodTicks = 60 + GetRandomValue(0, 20) * 6;
        agents[i].econ.productionTick        = agents[i].econ.productionPeriodTicks;

        agents[i].sprite.spriteType    = GetRandomValue(0, WORKER_TYPE_COUNT - 1);
        agents[i].sprite.animFrame     = GetRandomValue(0, WORKER_WALK_FRAMES - 1);
        agents[i].sprite.animTick      = ANIM_TICKS;
        agents[i].sprite.facing        = DIR_SOUTH;
        agents[i].sprite.tradeFlashTick = 0;

        agents_pick_new_target(agents, i, count, worldWidth, worldHeight);
    }
}

// ---------------------------------------------------------------------------
// Per-tick movement
// ---------------------------------------------------------------------------

#define WAYPOINT_RADIUS 10.0f

void agent_move(Agent *a) {
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
        a->body.x += dx * inv * AGENT_SPEED_PER_TICK;
        a->body.y += dy * inv * AGENT_SPEED_PER_TICK;
        if (fabsf(dx) >= fabsf(dy))
            a->sprite.facing = (dx > 0.0f) ? DIR_EAST : DIR_WEST;
        else
            a->sprite.facing = (dy > 0.0f) ? DIR_SOUTH : DIR_NORTH;
    }
}

// ---------------------------------------------------------------------------
// Per-tick sprite animation
// ---------------------------------------------------------------------------

void agent_update_sprite(Agent *a) {
    a->sprite.animTick--;
    if (a->sprite.animTick <= 0) {
        a->sprite.animFrame = (a->sprite.animFrame + 1) % WORKER_WALK_FRAMES;
        a->sprite.animTick  = ANIM_TICKS;
    }
    if (a->sprite.tradeFlashTick > 0)
        a->sprite.tradeFlashTick--;
}

// ---------------------------------------------------------------------------
// Proximity check, gossip, trade, and target selection
// ---------------------------------------------------------------------------

void agent_attempt_trade(Agent *agents, int i, int count, float worldW, float worldH) {
    Agent *a = &agents[i];

    if (a->body.targetType == TARGET_AGENT) {
        int bi = a->body.targetId;
        Agent *b = &agents[bi];
        float dx = a->body.x - b->body.x;
        float dy = a->body.y - b->body.y;
        if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
            for (int mid = 0; mid < MARKET_COUNT; mid++) {
                MarketId m = (MarketId)mid;
                market_gossip(a, b, m);
                market_trade(a, b, m);
            }
            agents_pick_new_target(agents, i,  count, worldW, worldH);
            agents_pick_new_target(agents, bi, count, worldW, worldH);
        }
    } else if (a->body.targetType == TARGET_WORK_CHOP ||
               a->body.targetType == TARGET_WORK_BUILD) {
        float dx = a->body.x - a->body.targetX;
        float dy = a->body.y - a->body.targetY;
        if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f)) {
            if (a->body.targetType == TARGET_WORK_CHOP)
                agent_execute_chop(a);
            else
                agent_execute_build(a);
            agents_pick_new_target(agents, i, count, worldW, worldH);
        }
    } else {
        float dx = a->body.x - a->body.targetX;
        float dy = a->body.y - a->body.targetY;
        if (dx*dx + dy*dy < (AGENT_RADIUS * 2.0f) * (AGENT_RADIUS * 2.0f))
            agents_pick_new_target(agents, i, count, worldW, worldH);
    }
}

// ---------------------------------------------------------------------------
// Sprite rendering
// ---------------------------------------------------------------------------

void draw_agent(const Agent *a, const Assets *assets) {
    int dirRow = (int)a->sprite.facing * WORKER_DIR_ROWS;
    int col    = a->sprite.animFrame;
    float srcX = (float)(col    * WORKER_FRAME_W);
    float srcY = (float)(dirRow * WORKER_FRAME_H);

    Rectangle src = { srcX, srcY, (float)WORKER_FRAME_W, (float)WORKER_FRAME_H };
    float disp = (float)AGENT_DISP;
    Rectangle dst = { a->body.x - disp * 0.5f,
                      a->body.y - disp * 0.5f,
                      disp, disp };

    Color tint = (a->sprite.tradeFlashTick > 0) ? YELLOW : WHITE;
    DrawTexturePro(assets->workers[a->sprite.spriteType % WORKER_TYPE_COUNT],
                   src, dst, (Vector2){0,0}, 0.0f, tint);
}

// ---------------------------------------------------------------------------
// Asset loading
// ---------------------------------------------------------------------------

static const char *WORKER_PATHS[WORKER_TYPE_COUNT] = {
    "assets/MiniWorldSprites/Characters/Workers/CyanWorker/FarmerCyan.png",
    "assets/MiniWorldSprites/Characters/Workers/LimeWorker/FarmerLime.png",
    "assets/MiniWorldSprites/Characters/Workers/PurpleWorker/FarmerPurple.png",
    "assets/MiniWorldSprites/Characters/Workers/RedWorker/FarmerRed.png",
};

void assets_load(Assets *a) {
    for (int i = 0; i < WORKER_TYPE_COUNT; i++) {
        a->workers[i] = LoadTexture(WORKER_PATHS[i]);
        SetTextureFilter(a->workers[i], TEXTURE_FILTER_POINT);
    }
}

void assets_unload(Assets *a) {
    for (int i = 0; i < WORKER_TYPE_COUNT; i++)
        UnloadTexture(a->workers[i]);
}
