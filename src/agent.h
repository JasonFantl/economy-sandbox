#ifndef AGENT_H
#define AGENT_H

#define MAX_AGENTS    200
#define AGENT_RADIUS  5.0f
#define AGENT_SPEED   120.0f
#define BELIEF_VOLATILITY 0.5f

typedef enum { TARGET_AGENT = 0, TARGET_POS = 1 } TargetType;

typedef struct {
    int        id;
    float      x;
    // Economy
    float      money;
    int        goods;
    float      basePersonalValue;  // S: marginal utility of the first unit
    float      halfValueAt;        // D: goods owned when marginal value halves
    float      expectedMarketValue;
    float      timeSinceLastTrade;
    float      maxTimeSinceLastTrade;
    // Movement target
    int        targetId;
    float      targetX;
    TargetType targetType;
    // Visual
    float      tradeFlash;
    int        spriteType;
    int        animFrame;
    float      animTimer;
    int        facingRight;
} Agent;

// Marginal utility of buying one more good (diminishing returns)
static inline float agent_potential_value(const Agent *a) {
    float r = (float)(a->goods + 1) / a->halfValueAt;
    return a->basePersonalValue / (r * r * r + 1.0f);
}

// Marginal utility of the last good owned (minimum acceptable sell price)
static inline float agent_current_value(const Agent *a) {
    float r = (float)a->goods / a->halfValueAt;
    return a->basePersonalValue / (r * r * r + 1.0f);
}

// Wants to buy: next good worth more than current asking price
static inline int agent_is_buyer(const Agent *a) {
    return agent_potential_value(a) > a->expectedMarketValue;
}

// Wants to sell: current good worth less than current asking price, and has goods
static inline int agent_is_seller(const Agent *a) {
    return a->goods > 0 && agent_current_value(a) < a->expectedMarketValue;
}

void agents_init(Agent *agents, int count, float worldWidth);
void agents_update(Agent *agents, int count, float dt);
void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth);
// Shift basePersonalValue of n randomly chosen agents by delta
void agents_influence(Agent *agents, int count, int n, float delta);

#endif
