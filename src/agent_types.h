#ifndef AGENT_TYPES_H
#define AGENT_TYPES_H

#include <stdint.h>
#include "world.h"

#define MAX_AGENTS        200
#define AGENT_RADIUS      5.0f
#define AGENT_SPEED       120.0f
#define MAX_PATH_LEN      256   // max waypoints stored per agent
#define WOOD_PER_CHAIR    4
#define MONEY_MAX_UTILITY 1000.0f  // marginal utility of $1 when money = 0

// Per-unit, per-second decay probabilities (editable at runtime)
extern float g_wood_decay_rate;
extern float g_chair_decay_rate;

typedef enum { MARKET_WOOD = 0, MARKET_CHAIR = 1, MARKET_COUNT = 2 } MarketId;
typedef enum { ACTION_LEISURE = 0, ACTION_CHOP = 1, ACTION_BUILD = 2 } AgentAction;
typedef enum {
    TARGET_AGENT      = 0,  // traveling to shared trade-meeting tile
    TARGET_POS        = 1,  // traveling to random solo tile
    TARGET_WORK_CHOP  = 2,  // traveling to wood-cutting site; execute chop on arrival
    TARGET_WORK_BUILD = 3,  // traveling to chair-making site; execute build on arrival
} TargetType;

// ---------------------------------------------------------------------------
// Economic building blocks
// ---------------------------------------------------------------------------

// Per-market state: inventory + price belief
typedef struct {
    int    goods;
    float  maxUtility;          // peak marginal utility (at 0 goods owned)
    float  minUtility;          // floor marginal utility (as goods → ∞)
    float  halfSaturation;      // goods quantity where utility is halfway between min and max
    float  priceExpectation;    // Nerlove adaptive expected price ($)
    float  frustrationTimer;    // time waiting to trade while willing
    float  frustrationThreshold;// threshold before adjusting price expectation
} AgentMarket;

// Leisure preference — utility of resting, diminishing with consecutive idle time
typedef struct {
    float  maxUtility;    // peak leisure utility (when rested)
    float  minUtility;    // floor leisure utility (even when very bored)
    float  halfSaturation;// idle seconds at which leisure utility is halfway
    float  idleTime;      // accumulated idle seconds; resets on productive action
} LeisureState;

// ---------------------------------------------------------------------------
// Agent sub-structs — one per concern
// ---------------------------------------------------------------------------

// Four cardinal directions for sprite row selection
typedef enum { DIR_SOUTH = 0, DIR_WEST = 1, DIR_EAST = 2, DIR_NORTH = 3 } FacingDir;

// World / movement state
typedef struct {
    int        id;
    float      x, y;
    int        targetId;           // trade partner index (TARGET_AGENT)
    float      targetX, targetY;   // final destination pixel (exact tile centre)
    TargetType targetType;
    // Pre-computed waypoint path.  Filled once by BFS at target-pick time;
    // per-step movement is O(1) — just advance along the list.
    float      wpPx[MAX_PATH_LEN];
    float      wpPy[MAX_PATH_LEN];
    int        wpCount;
    int        wpIdx;
} AgentBody;

// Economic decision-making state
typedef struct {
    float        money;
    AgentMarket  markets[MARKET_COUNT];
    LeisureState leisure;
    AgentAction  lastAction;
    AgentAction  pendingWork;      // work decided but not yet executed (ACTION_LEISURE = none)
    float        beliefUpdateRate; // Nerlove alpha: how quickly new prices are incorporated [0,1]
    float        productionTimer;  // countdown to next production decision
    float        productionPeriod; // seconds between production decisions
} AgentEcon;

// Rendering / animation state
typedef struct {
    float     tradeFlash;
    int       spriteType;  // which worker color variant (0-3)
    int       animFrame;   // 0..WORKER_WALK_FRAMES-1
    float     animTimer;
    FacingDir facing;
} AgentSprite;

// ---------------------------------------------------------------------------
// Agent — composed of the three sub-structs
// ---------------------------------------------------------------------------

typedef struct {
    AgentBody   body;
    AgentEcon   econ;
    AgentSprite sprite;
} Agent;

#define AGENT_MKT(a, mid) (&(a)->econ.markets[mid])

#endif
