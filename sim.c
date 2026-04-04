#include "sim.h"
#include "econ/agent.h"
#include "econ/market.h"

#define PRICE_RECORD_INTERVAL 15   // record prices every 15 ticks (0.25s at 60 ticks/s)

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

    if (++g_simulation.priceTick >= PRICE_RECORD_INTERVAL) {
        for (int mid = 0; mid < MARKET_COUNT; mid++) {
            MarketId m = (MarketId)mid;
            avh_record_prices(&g_simulation.avh[mid], g_simulation.agents, g_simulation.count, m);
            avh_record_personal_valuations(&g_simulation.pvh[mid], g_simulation.agents, g_simulation.count, m);
            avh_record_goods(&g_simulation.gvh[mid], g_simulation.agents, g_simulation.count, m);
        }
        g_simulation.priceTick = 0;
    }
}

void sim_update(void) {
    if (g_simulation.paused) return;
    for (int s = 0; s < g_simulation.ticks_per_frame; s++)
        sim_step();
}
