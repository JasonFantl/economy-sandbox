#ifndef AGENT_H
#define AGENT_H

#define MAX_AGENTS    200
#define AGENT_RADIUS  5.0f
#define AGENT_SPEED   120.0f

typedef enum { TARGET_AGENT = 0, TARGET_POS = 1 } TargetType;

typedef struct {
    int        id;
    float      x;
    float      personalValue;
    float      expectedMarketValue;
    int        targetId;    // used when targetType == TARGET_AGENT
    float      targetX;     // used when targetType == TARGET_POS
    TargetType targetType;
    float      tradeFlash;
} Agent;

void agents_init(Agent *agents, int count, float worldWidth);
void agents_update(Agent *agents, int count, float dt);
void agents_pick_new_target(Agent *agent, int agentCount, float worldWidth);

#endif
