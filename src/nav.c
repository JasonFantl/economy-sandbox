#include "nav.h"
#include "econ.h"
#include "raylib.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Navigation graph
// ---------------------------------------------------------------------------
// Walkable tiles: bare ground (grass / path / dead-grass) with TILE_NONE on
// the object layer, OR a bridge tile on the object layer.
// Agents walk only on these tiles; everything else (buildings, trees, water)
// is impassable.
//
// BFS is run once per target-pick; per-step movement is O(1).

#define MAX_NAV         16384
#define WAYPOINT_RADIUS  10.0f  // px — advance to next waypoint when this close
#define WAYPOINT_JITTER  12     // px — max random offset within each intermediate tile

static int     g_nav_count = 0;
static int16_t g_nav_tile_x[MAX_NAV];
static int16_t g_nav_tile_y[MAX_NAV];
static int16_t *g_nav_map = NULL;   // [y*mapW + x] → nav index, -1 = impassable
static int     g_nav_mapW = 0, g_nav_mapH = 0;

// Work-site nav indices (assigned once at nav_init)
static int g_chop_nav_idx  = -1;
static int g_build_nav_idx = -1;

// Public pixel positions for optional rendering markers
float g_chop_site_px  = 0.0f, g_chop_site_py  = 0.0f;
float g_build_site_px = 0.0f, g_build_site_py = 0.0f;

// BFS scratch (single-threaded only)
static int8_t  g_bfs_vis[MAX_NAV];
static int16_t g_bfs_par_x[MAX_NAV];
static int16_t g_bfs_par_y[MAX_NAV];
static int     g_bfs_queue[MAX_NAV];

static inline float tile_cx(int tx) {
    return ((float)tx + 0.5f) * (float)TILE_SIZE * WORLD_TILE_SCALE;
}
static inline float tile_cy(int ty) {
    return ((float)ty + 0.5f) * (float)TILE_SIZE * WORLD_TILE_SCALE;
}

// Walkability predicate: only bare ground tiles + bridges
static bool tile_walkable(const MapCell *ground, const MapCell *obj) {
    if (!ground) return false;
    // Bridge on the object layer → walkable regardless of ground
    if (obj && obj->type == TILE_BRIDGE) return true;
    // Anything else on the object layer blocks movement
    if (obj && obj->type != TILE_NONE) return false;
    // Ground must be a grass-family or path tile (no water, trees, buildings)
    return (ground->type == TILE_GRASS     ||
            ground->type == TILE_GRASS_ALT ||
            ground->type == TILE_PATH      ||
            ground->type == TILE_DEAD_GRASS);
}

// Nav index of the walkable tile whose centre is nearest to (wx, wy).
// O(1) fast-path when agent is already on walkable ground; O(N) fallback.
static int nearest_nav_idx(float wx, float wy) {
    if (g_nav_count == 0) return -1;
    float tpx = (float)TILE_SIZE * WORLD_TILE_SCALE;
    int tx = (int)(wx / tpx), ty = (int)(wy / tpx);
    if (tx >= 0 && tx < g_nav_mapW && ty >= 0 && ty < g_nav_mapH) {
        int16_t idx = g_nav_map[ty * g_nav_mapW + tx];
        if (idx >= 0) return (int)idx;
    }
    float ftx = wx / tpx, fty = wy / tpx;
    int best = 0; float bestD2 = 1e30f;
    for (int i = 0; i < g_nav_count; i++) {
        float dx = (float)g_nav_tile_x[i] - ftx;
        float dy = (float)g_nav_tile_y[i] - fty;
        float d2 = dx*dx + dy*dy;
        if (d2 < bestD2) { bestD2 = d2; best = i; }
    }
    return best;
}

// BFS from srcIdx to dstIdx.  Writes jittered world-pixel waypoints into
// outPx/outPy (max maxLen entries, excluding start, including end).
// Intermediate waypoints get a random ±WAYPOINT_JITTER offset within the tile;
// the final waypoint is the exact tile centre so two partners converge there.
// Returns the number of waypoints written (0 = no path or same tile).
static int bfs_path(int srcIdx, int dstIdx,
                    float *outPx, float *outPy, int maxLen) {
    if (srcIdx < 0 || dstIdx < 0) return 0;
    if (srcIdx == dstIdx) {
        if (maxLen > 0) {
            outPx[0] = tile_cx(g_nav_tile_x[srcIdx]);
            outPy[0] = tile_cy(g_nav_tile_y[srcIdx]);
            return 1;
        }
        return 0;
    }

    memset(g_bfs_vis, 0, (size_t)g_nav_count);
    g_bfs_vis[srcIdx] = 1;
    g_bfs_par_x[srcIdx] = g_nav_tile_x[srcIdx];
    g_bfs_par_y[srcIdx] = g_nav_tile_y[srcIdx];

    int head = 0, tail = 0;
    g_bfs_queue[tail++] = srcIdx;

    static const int DDX[4] = {1, -1,  0, 0};
    static const int DDY[4] = {0,  0,  1, -1};

    bool found = false;
    while (head < tail && !found) {
        int cidx = g_bfs_queue[head++];
        int cx = g_nav_tile_x[cidx], cy = g_nav_tile_y[cidx];
        for (int d = 0; d < 4; d++) {
            int nx = cx + DDX[d], ny = cy + DDY[d];
            if (nx < 0 || nx >= g_nav_mapW || ny < 0 || ny >= g_nav_mapH) continue;
            int nidx = g_nav_map[ny * g_nav_mapW + nx];
            if (nidx < 0 || g_bfs_vis[nidx]) continue;
            g_bfs_vis[nidx] = 1;
            g_bfs_par_x[nidx] = (int16_t)cx;
            g_bfs_par_y[nidx] = (int16_t)cy;
            if (nidx == dstIdx) { found = true; break; }
            if (tail < MAX_NAV) g_bfs_queue[tail++] = nidx;
        }
    }
    if (!found) return 0;

    // Reconstruct path (destination → source) into temporary tile arrays
    static int16_t tmpX[MAX_PATH_LEN], tmpY[MAX_PATH_LEN];
    int len = 0;
    int cur = dstIdx;
    while (cur != srcIdx && len < MAX_PATH_LEN) {
        tmpX[len] = g_nav_tile_x[cur];
        tmpY[len] = g_nav_tile_y[cur];
        len++;
        int16_t px = g_bfs_par_x[cur], py = g_bfs_par_y[cur];
        cur = (int)g_nav_map[(int)py * g_nav_mapW + (int)px];
        if (cur < 0) break;
    }

    // Reverse into output, applying pixel jitter to intermediate waypoints
    int count = (len < maxLen) ? len : maxLen;
    for (int i = 0; i < count; i++) {
        int ti = len - 1 - i;  // source-to-dest order
        float cx = tile_cx(tmpX[ti]);
        float cy = tile_cy(tmpY[ti]);
        bool isLast = (i == count - 1);
        if (!isLast) {
            cx += (float)GetRandomValue(-WAYPOINT_JITTER, WAYPOINT_JITTER);
            cy += (float)GetRandomValue(-WAYPOINT_JITTER, WAYPOINT_JITTER);
        }
        outPx[i] = cx;
        outPy[i] = cy;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Public: build nav graph from map (call once at startup)
// ---------------------------------------------------------------------------

void agents_nav_init(const WorldMap *map) {
    if (!map) return;
    if (g_nav_map) { free(g_nav_map); g_nav_map = NULL; }
    g_nav_mapW = map->width;
    g_nav_mapH = map->height;
    int n = map->width * map->height;
    g_nav_map = malloc((size_t)n * sizeof(int16_t));
    if (!g_nav_map) return;
    for (int i = 0; i < n; i++) g_nav_map[i] = -1;
    g_nav_count = 0;

    for (int ty = 0; ty < map->height; ty++) {
        for (int tx = 0; tx < map->width; tx++) {
            const MapCell *ground = worldmap_cell(map, tx, ty);
            const MapCell *obj    = worldmap_obj_cell(map, tx, ty);
            if (!tile_walkable(ground, obj)) continue;
            if (g_nav_count >= MAX_NAV) continue;
            g_nav_map[ty * map->width + tx] = (int16_t)g_nav_count;
            g_nav_tile_x[g_nav_count] = (int16_t)tx;
            g_nav_tile_y[g_nav_count] = (int16_t)ty;
            g_nav_count++;
        }
    }

    // Assign two distinct random walkable tiles as the wood-cutting and chair-making sites
    if (g_nav_count >= 2) {
        g_chop_nav_idx  = GetRandomValue(0, g_nav_count - 1);
        do { g_build_nav_idx = GetRandomValue(0, g_nav_count - 1); }
        while (g_build_nav_idx == g_chop_nav_idx);

        g_chop_site_px  = tile_cx(g_nav_tile_x[g_chop_nav_idx]);
        g_chop_site_py  = tile_cy(g_nav_tile_y[g_chop_nav_idx]);
        g_build_site_px = tile_cx(g_nav_tile_x[g_build_nav_idx]);
        g_build_site_py = tile_cy(g_nav_tile_y[g_build_nav_idx]);
    }
}

// ---------------------------------------------------------------------------
// Internal helper: assign a BFS path to one agent toward a specific nav tile
// ---------------------------------------------------------------------------

static void assign_path(Agent *a, int dstNavIdx) {
    int srcNavIdx = nearest_nav_idx(a->body.x, a->body.y);
    a->body.wpCount = bfs_path(srcNavIdx, dstNavIdx,
                               a->body.wpPx, a->body.wpPy, MAX_PATH_LEN);
    a->body.wpIdx  = 0;
    a->body.targetX = tile_cx(g_nav_tile_x[dstNavIdx]);
    a->body.targetY = tile_cy(g_nav_tile_y[dstNavIdx]);
}

// ---------------------------------------------------------------------------
// Public helpers
// ---------------------------------------------------------------------------

bool nav_random_position(float *px, float *py) {
    if (g_nav_count <= 0) return false;
    int idx = GetRandomValue(0, g_nav_count - 1);
    *px = tile_cx(g_nav_tile_x[idx]);
    *py = tile_cy(g_nav_tile_y[idx]);
    return true;
}

// ---------------------------------------------------------------------------
// Target picking — BFS runs once here; per-step movement is O(1)
// ---------------------------------------------------------------------------

void agents_pick_new_target(Agent *agents, int agentIdx, int agentCount,
                             float worldWidth, float worldHeight) {
    Agent *agent = &agents[agentIdx];

    // Pending work takes priority over trading and random movement
    if (agent->econ.pendingWork == ACTION_CHOP && g_chop_nav_idx >= 0) {
        agent->body.targetType  = TARGET_WORK_CHOP;
        assign_path(agent, g_chop_nav_idx);
        agent->econ.pendingWork = ACTION_LEISURE;  // clear flag
        return;
    }
    if (agent->econ.pendingWork == ACTION_BUILD && g_build_nav_idx >= 0) {
        agent->body.targetType  = TARGET_WORK_BUILD;
        assign_path(agent, g_build_nav_idx);
        agent->econ.pendingWork = ACTION_LEISURE;
        return;
    }

    // Try to set up a trade meeting (50 % of the time when nav data is available)
    if (g_nav_count > 1 && GetRandomValue(0, 1) == 0) {
        // Collect candidates: agents not already heading to a trade meeting
        static int cand[MAX_AGENTS];
        int ncand = 0;
        for (int i = 0; i < agentCount; i++) {
            if (i != agentIdx && agents[i].body.targetType != TARGET_AGENT)
                cand[ncand++] = i;
        }

        if (ncand > 0) {
            int partnerIdx = cand[GetRandomValue(0, ncand - 1)];
            Agent *partner = &agents[partnerIdx];

            // Pick a random meeting tile (not the current tile of either agent)
            int meetNav;
            int srcA = nearest_nav_idx(agent->body.x, agent->body.y);
            int srcB = nearest_nav_idx(partner->body.x, partner->body.y);
            int tries = 0;
            do {
                meetNav = GetRandomValue(0, g_nav_count - 1);
                tries++;
            } while (tries < 8 && (meetNav == srcA || meetNav == srcB));

            // Assign paths for both agents toward the same meeting tile
            agent->body.targetType  = TARGET_AGENT;
            agent->body.targetId    = partnerIdx;
            assign_path(agent, meetNav);

            partner->body.targetType = TARGET_AGENT;
            partner->body.targetId   = agentIdx;
            assign_path(partner, meetNav);
            return;
        }
    }

    // Solo movement: navigate to a random walkable tile
    agent->body.targetType = TARGET_POS;
    agent->body.targetId   = -1;

    if (g_nav_count > 0) {
        int dstNav = GetRandomValue(0, g_nav_count - 1);
        assign_path(agent, dstNav);
    } else {
        agent->body.targetX = (float)GetRandomValue(20, (int)worldWidth  - 20);
        agent->body.targetY = (float)GetRandomValue(20, (int)worldHeight - 20);
        agent->body.wpCount = 0;
        agent->body.wpIdx   = 0;
    }
}
