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
    // Terrain (not walkable)
    TILE_WATER        = 3,
    TILE_TREE         = 4,
    TILE_PINE_TREE    = 5,
    TILE_ROCK         = 6,
    // Buildings — each carries an economic role
    TILE_HUT          = 7,   // simple dwelling: agents rest here
    TILE_HOUSE        = 8,   // dwelling: agents rest here
    TILE_MARKET       = 9,   // trading location
    TILE_WORKSHOP     = 10,  // production (wood → chair)
    TILE_RESOURCE     = 11,  // resource gathering / stockpile
    TILE_TAVERN       = 12,  // leisure location
    TILE_COUNT        = 13
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
