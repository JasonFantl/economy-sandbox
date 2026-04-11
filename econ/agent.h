#ifndef AGENT_H
#define AGENT_H

#include "raylib.h"
#include "world/world.h"
#include "world/tileset.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_AGENTS        200
#define AGENT_RADIUS      5.0f
#define MAX_PATH_LEN      256
extern int g_wood_per_chair;   // wood units consumed per chair built (default 4)
#define MONEY_MAX_UTILITY 1000.0f

#define TICKS_PER_SECOND    60
#define AGENT_SPEED_PER_TICK 2.0f   // pixels per tick (120 px/s ÷ 60)
#define ANIM_TICKS           9      // frames between sprite frame advance (0.15s)
#define TRADE_FLASH_TICKS   24      // ticks the trade highlight lasts (0.4s)

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
} AgentMarket;

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
    float        leisureUtility;
    AgentAction  lastAction;
    AgentAction  pendingWork;
    float        beliefUpdateRate;
    int          productionTick;
    int          productionPeriodTicks;
} AgentEcon;

// ---------------------------------------------------------------------------
// Worker sprite sheet constants
// Sheet: 80x192 px = 5 cols x 12 rows of 16x16 frames
// ---------------------------------------------------------------------------
#define WORKER_FRAME_W     16
#define WORKER_FRAME_H     16
#define WORKER_COLS         5
#define WORKER_WALK_FRAMES  3
#define WORKER_DIR_ROWS     3
#define WORKER_TYPE_COUNT   4

typedef struct {
    int       tradeFlashTick;
    int       spriteType;
    int       animFrame;
    int       animTick;
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

// Per-tick update functions (called once per tick per agent)
void agent_move(Agent *a);
void agent_update_sprite(Agent *a);

// Interaction — checks proximity, gossips, trades, picks new targets
void agent_attempt_trade(Agent *agents, int i, int count, float worldW, float worldH);

// Sprite rendering (call inside BeginMode2D/EndMode2D)
void draw_agent(const Agent *a, const Assets *assets);

// Asset loading
void assets_load(Assets *a);
void assets_unload(Assets *a);

#endif
