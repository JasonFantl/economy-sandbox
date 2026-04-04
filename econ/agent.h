#ifndef AGENT_H
#define AGENT_H

#include "raylib.h"
#include "world/world.h"
#include "world/tileset.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_AGENTS        200
#define AGENT_RADIUS      5.0f
#define AGENT_SPEED       120.0f
#define MAX_PATH_LEN      256
#define WOOD_PER_CHAIR    4
#define MONEY_MAX_UTILITY 1000.0f

// Per-unit, per-second decay probabilities (editable at runtime)
extern float g_wood_decay_rate;
extern float g_chair_decay_rate;

typedef enum { MARKET_WOOD = 0, MARKET_CHAIR = 1, MARKET_COUNT = 2 } MarketId;
typedef enum { ACTION_LEISURE = 0, ACTION_CHOP = 1, ACTION_BUILD = 2 } AgentAction;
typedef enum {
    TARGET_AGENT      = 0,
    TARGET_POS        = 1,
    TARGET_WORK_CHOP  = 2,
    TARGET_WORK_BUILD = 3,
} TargetType;

typedef struct {
    int    goods;
    float  maxUtility;
    float  minUtility;
    float  halfSaturation;
    float  priceExpectation;
    float  frustrationTimer;
    float  frustrationThreshold;
} AgentMarket;

typedef struct {
    float  maxUtility;
    float  minUtility;
    float  halfSaturation;
    float  idleTime;
} LeisureState;

typedef enum { DIR_SOUTH = 0, DIR_WEST = 1, DIR_EAST = 2, DIR_NORTH = 3 } FacingDir;

typedef struct {
    int        id;
    float      x, y;
    int        targetId;
    float      targetX, targetY;
    TargetType targetType;
    float      wpPx[MAX_PATH_LEN];
    float      wpPy[MAX_PATH_LEN];
    int        wpCount;
    int        wpIdx;
} AgentBody;

typedef struct {
    float        money;
    AgentMarket  markets[MARKET_COUNT];
    LeisureState leisure;
    AgentAction  lastAction;
    AgentAction  pendingWork;
    float        beliefUpdateRate;
    float        productionTimer;
    float        productionPeriod;
} AgentEcon;

typedef struct {
    float     tradeFlash;
    int       spriteType;
    int       animFrame;
    float     animTimer;
    FacingDir facing;
} AgentSprite;

typedef struct {
    AgentBody   body;
    AgentEcon   econ;
    AgentSprite sprite;
} Agent;

#define AGENT_MKT(a, mid) (&(a)->econ.markets[mid])
#define AGENT_DISP ((int)(WORKER_FRAME_W))

// Agent lifecycle
void agents_init(Agent *agents, int count, float worldWidth, float worldHeight);
void agents_update(Agent *agents, int count, float dt);

// Sprite rendering (call inside BeginMode2D/EndMode2D)
void draw_agent(const Agent *a, const Assets *assets);

#endif
