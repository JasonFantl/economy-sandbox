#ifndef AGENT_H
#define AGENT_H

#define MAX_AGENTS        200
#define AGENT_RADIUS        5.0f
#define AGENT_SPEED         120.0f
#define WOOD_PER_CHAIR      4
#define MONEY_UTILITY_BASE  1000.0f  // utility value of $1 when money→0

// Per-unit, per-second break probabilities (editable at runtime)
extern float g_wood_break_prob;
extern float g_chair_break_prob;

typedef enum { MARKET_WOOD = 0, MARKET_CHAIR = 1, MARKET_COUNT = 2 } MarketId;
typedef enum { ACTION_LEISURE = 0, ACTION_CHOP = 1, ACTION_BUILD = 2 } AgentAction;
typedef enum { TARGET_AGENT = 0, TARGET_POS = 1 } TargetType;

// ---------------------------------------------------------------------------
// Economic building blocks
// ---------------------------------------------------------------------------

// Per-market economy state
typedef struct {
    int    goods;
    float  basePersonalValue;
    float  minValue;      // floor: value never drops below this
    float  halfValueAt;
    float  expectedMarketValue;
    float  timeSinceLastTrade;
    float  maxTimeSinceLastTrade;
} AgentMarket;

// Leisure preference state (value diminishes with consecutive idle time)
typedef struct {
    float  basePersonalValue;
    float  minValue;      // floor: minimum leisure value even after long idle streaks
    float  halfValueAt;   // seconds of idling until value is halfway between base and min
    float  idleTime;      // seconds spent idle; resets on productive action
} LeisureState;

// ---------------------------------------------------------------------------
// Agent sub-structs — one per concern
// ---------------------------------------------------------------------------

// World/movement state
typedef struct {
    int        id;
    float      x;
    int        targetId;
    float      targetX;
    TargetType targetType;
} AgentBody;

// Economic decision-making state
typedef struct {
    float        money;
    AgentMarket  markets[MARKET_COUNT];
    LeisureState leisure;
    AgentAction  lastAction;
    float        alpha;              // Nerlove belief update speed [0,1]
    float        actionTimer;        // time until next production decision
    float        productionInterval; // seconds between production decisions
} AgentEcon;

// Rendering / animation state
typedef struct {
    float tradeFlash;
    int   spriteType;
    int   animFrame;
    float animTimer;
    int   facingRight;
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

// ---------------------------------------------------------------------------
// Inline utility / value functions
// ---------------------------------------------------------------------------

// Marginal utility of buying one more unit of this good (diminishing returns)
static inline float market_potential_value(const AgentMarket *m) {
    float r = (float)(m->goods + 1) / m->halfValueAt;
    return m->minValue + (m->basePersonalValue - m->minValue) / (r * r * r + 1.0f);
}

// Marginal utility of the last unit owned (minimum acceptable sell price)
static inline float market_current_value(const AgentMarket *m) {
    float r = (float)m->goods / m->halfValueAt;
    return m->minValue + (m->basePersonalValue - m->minValue) / (r * r * r + 1.0f);
}

// Value of doing nothing (diminishes the longer the agent has been idle)
static inline float leisure_value(const LeisureState *l) {
    float r = l->idleTime / l->halfValueAt;
    return l->minValue + (l->basePersonalValue - l->minValue) / (r * r * r + 1.0f);
}

// Utility an agent derives from $1 given their current wealth
static inline float utility_per_dollar(float money) {
    return MONEY_UTILITY_BASE / (money + 1.0f);
}

// Buyer if the utility of acquiring the good exceeds the utility of the money spent
static inline int market_is_buyer(const AgentMarket *m, float money) {
    return market_potential_value(m) > m->expectedMarketValue * utility_per_dollar(money);
}

// Seller if the utility of the money received exceeds the utility of keeping the good
static inline int market_is_seller(const AgentMarket *m, float money) {
    return market_current_value(m) < m->expectedMarketValue * utility_per_dollar(money);
}

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------

// agent.c — world/body orchestration
void agents_init(Agent *agents, int count, float worldWidth);
void agents_update(Agent *agents, int count, float dt);
void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth);

// econ.c — economic logic
void agents_influence(Agent *agents, int count, int n, float delta, MarketId mid);
void agents_give_money(Agent *agents, int count, int n, float delta);
void agents_give_goods(Agent *agents, int count, int n, int delta, MarketId mid);

#endif
