#ifndef AGENT_H
#define AGENT_H

#define MAX_AGENTS    200
#define AGENT_RADIUS  5.0f
#define AGENT_SPEED   120.0f
#define BELIEF_VOLATILITY 0.5f  // shared by agent (idle timer) and market (trade)

typedef enum { TARGET_AGENT = 0, TARGET_POS = 1 } TargetType;

typedef struct {
    int        id;
    float      x;
    float      personalValue;
    float      expectedMarketValue;
    int        targetId;
    float      targetX;
    TargetType targetType;
    float      tradeFlash;
    float      timeSinceLastTrade;     // sim-seconds since last successful trade
    float      maxTimeSinceLastTrade;  // threshold before EMV is adjusted
} Agent;

void agents_init(Agent *agents, int count, float worldWidth);
void agents_update(Agent *agents, int count, float dt);
void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth);
// Shift personalValue of n randomly chosen agents by delta
void agents_influence(Agent *agents, int count, int n, float delta);

#endif
