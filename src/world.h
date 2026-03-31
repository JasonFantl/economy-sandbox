#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <stdbool.h>

#define TILE_SIZE 16  // base pixel size of one tile

// ---------------------------------------------------------------------------
// Tile types — shared between game and map builder
// ---------------------------------------------------------------------------

typedef enum {
    // Ground (walkable)
    TILE_GRASS        = 0,
    TILE_GRASS_ALT    = 1,
    TILE_PATH         = 2,
    TILE_DEAD_GRASS   = 3,   // Ground/DeadGrass.png  48×32 — 3×2 = 6 variants
    // Terrain (not walkable)
    TILE_WATER        = 4,
    TILE_TREE         = 5,
    TILE_PINE_TREE    = 6,
    TILE_ROCK         = 7,
    TILE_WHEATFIELD   = 8,   // Nature/Wheatfield.png 64×16 — 4 variants (walkable, resource)
    // Buildings — each carries an economic role
    TILE_HUT          = 9,   // simple dwelling: agents rest here
    TILE_HOUSE        = 10,  // dwelling: agents rest here
    TILE_MARKET       = 11,  // trading location
    TILE_WORKSHOP     = 12,  // production (wood → chair)
    TILE_RESOURCE     = 13,  // resource gathering / stockpile
    TILE_TAVERN       = 14,  // leisure location
    // Miscellaneous
    TILE_BRIDGE       = 15,  // Miscellaneous/Bridge.png  80×48 — 5×3 = 15 variants (walkable)
    TILE_WELL         = 16,  // Miscellaneous/Well.png    48×32 — 3×2 = 6 variants
    TILE_CHEST        = 17,  // Miscellaneous/Chests.png  32×16 — 2 variants
    TILE_SIGN         = 18,  // Miscellaneous/Signs.png   64×16 — 4 variants (walkable)
    TILE_COUNT        = 19
} TileType;

// Static metadata for each tile type — walkability and economic role
typedef struct {
    const char *label;
    bool        walkable;
    bool        isBuilding;
    // Economic roles (used by the simulation)
    bool        isHome;
    bool        isMarket;
    bool        isWorkshop;
    bool        isResource;
    bool        isLeisure;
} TileInfo;

extern const TileInfo TILE_INFO[TILE_COUNT];

// ---------------------------------------------------------------------------
// Map cells and world map
// ---------------------------------------------------------------------------

typedef struct {
    uint8_t type;     // TileType — what kind of tile
    uint8_t variant;  // visual variant index within that tile type
} MapCell;

typedef struct {
    int      width, height;
    MapCell *cells;           // row-major: cells[y * width + x]
} WorldMap;

// ---------------------------------------------------------------------------
// Map lifecycle and I/O
// Binary format: magic "ECON", uint16 version=1, uint16 w, uint16 h,
//                then (w*h) MapCell entries (2 bytes each)
// ---------------------------------------------------------------------------

WorldMap *worldmap_create(int width, int height);   // filled with TILE_GRASS
void      worldmap_free(WorldMap *m);
MapCell  *worldmap_cell(const WorldMap *m, int x, int y); // NULL if out of bounds
bool      worldmap_save(const WorldMap *m, const char *path);
WorldMap *worldmap_load(const char *path);          // NULL on failure

#endif
