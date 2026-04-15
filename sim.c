#include "sim.h"
#include "econ/agent.h"
#include "econ/market.h"
#include <string.h>

SimState g_simulation = { .paused = false, .ticks_per_frame = 1 };

static void sim_step(void) {
    for (int i = 0; i < g_simulation.count; i++) {
        Agent *agent = &g_simulation.agents[i];
        agent_move(agent);
        agent_update_sprite(agent);
        agent_econ_update(agent);
        agent_attempt_trade(g_simulation.agents, i, g_simulation.count,
                            g_simulation.worldW, g_simulation.worldH);
    }
}

void sim_restart(void) {
    agents_init(g_simulation.agents, g_simulation.count,
                g_simulation.worldW, g_simulation.worldH);
    memset(g_simulation.avh, 0, sizeof(g_simulation.avh));
    memset(g_simulation.pvh, 0, sizeof(g_simulation.pvh));
    memset(g_simulation.gvh, 0, sizeof(g_simulation.gvh));
    memset(&g_simulation.mvh, 0, sizeof(g_simulation.mvh));
    memset(&g_simulation.speedHistory, 0, sizeof(g_simulation.speedHistory));
}

void sim_update(void) {
    if (g_simulation.paused) return;
    for (int s = 0; s < g_simulation.ticks_per_frame; s++)
        sim_step();
    // Record once per frame so each sample represents ticks_per_frame sim steps
    for (int mid = 0; mid < MARKET_COUNT; mid++) {
        MarketId m = (MarketId)mid;
        avh_record_prices(&g_simulation.avh[mid], g_simulation.agents, g_simulation.count, m);
        avh_record_personal_valuations(&g_simulation.pvh[mid], g_simulation.agents, g_simulation.count, m);
        avh_record_goods(&g_simulation.gvh[mid], g_simulation.agents, g_simulation.count, m);
    }
    avh_record_money(&g_simulation.mvh, g_simulation.agents, g_simulation.count);
    speed_history_record(&g_simulation.speedHistory, g_simulation.ticks_per_frame);
}
