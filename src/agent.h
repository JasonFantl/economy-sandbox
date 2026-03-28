#ifndef AGENT_H
#define AGENT_H

#define MAX_AGENTS    200
#define AGENT_RADIUS  5.0f
#define AGENT_SPEED   120.0f

typedef struct {
    int   id;
    float x;
    float personalValue;
    float expectedMarketValue;
    int   targetId;
    float tradeFlash; // visual highlight timer after trade
} Agent;

void agents_init(Agent *agents, int count, float worldWidth);
void agents_update(Agent *agents, int count, float dt);
int  agents_pick_target(int count, int selfId);

#endif
