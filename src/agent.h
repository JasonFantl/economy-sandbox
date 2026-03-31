#ifndef AGENT_H
#define AGENT_H

#define MAX_AGENTS       200
#define AGENT_RADIUS     5.0f
#define AGENT_SPEED      120.0f
#define WOOD_PER_CHAIR   4
#define MONEY_MAX_UTILITY 1000.0f  // marginal utility of $1 when money = 0

// Per-unit, per-second decay probabilities (editable at runtime)
extern float g_wood_decay_rate;
extern float g_chair_decay_rate;

typedef enum { MARKET_WOOD = 0, MARKET_CHAIR = 1, MARKET_COUNT = 2 } MarketId;
typedef enum { ACTION_LEISURE = 0, ACTION_CHOP = 1, ACTION_BUILD = 2 } AgentAction;
typedef enum { TARGET_AGENT = 0, TARGET_POS = 1 } TargetType;

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
    int        targetId;
    float      targetX, targetY;
    TargetType targetType;
} AgentBody;

// Economic decision-making state
typedef struct {
    float        money;
    AgentMarket  markets[MARKET_COUNT];
    LeisureState leisure;
    AgentAction  lastAction;
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

// ---------------------------------------------------------------------------
// Inline utility / value functions
// ---------------------------------------------------------------------------

// Marginal utility of buying one more unit (willingness to pay, in utility ₴)
static inline float marginal_buy_utility(const AgentMarket *m) {
    float r = (float)(m->goods + 1) / m->halfSaturation;
    return m->minUtility + (m->maxUtility - m->minUtility) / (r * r * r + 1.0f);
}

// Marginal utility of the last unit owned (minimum acceptable sell price, in utility ₴)
static inline float marginal_sell_utility(const AgentMarket *m) {
    float r = (float)m->goods / m->halfSaturation;
    return m->minUtility + (m->maxUtility - m->minUtility) / (r * r * r + 1.0f);
}

// Leisure utility (diminishes the longer the agent has been idle)
static inline float leisure_utility(const LeisureState *l) {
    float r = l->idleTime / l->halfSaturation;
    return l->minUtility + (l->maxUtility - l->minUtility) / (r * r * r + 1.0f);
}

// Marginal utility of $1 given the agent's current wealth
static inline float money_marginal_utility(float money) {
    return MONEY_MAX_UTILITY / (money + 1.0f);
}

// True if buying would yield more utility than spending the money elsewhere
static inline int wants_to_buy(const AgentMarket *m, float money) {
    return marginal_buy_utility(m) > m->priceExpectation * money_marginal_utility(money);
}

// True if selling would yield more utility than keeping the good
static inline int wants_to_sell(const AgentMarket *m, float money) {
    return marginal_sell_utility(m) < m->priceExpectation * money_marginal_utility(money);
}

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------

// agent.c — world/body orchestration
void agents_init(Agent *agents, int count, float worldWidth, float worldHeight);
void agents_update(Agent *agents, int count, float dt);
void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth, float worldHeight);

// econ.c — economic logic
void agents_adjust_valuations(Agent *agents, int count, int numAgents, float delta, MarketId mid);
void agents_inject_money(Agent *agents, int count, int numAgents, float delta);
void agents_inject_goods(Agent *agents, int count, int numAgents, int delta, MarketId mid);

#endif
