#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <stdbool.h>

#define TILE_SIZE        16     // base pixel size of one tile (pixels in source asset)
#define WORLD_TILE_SCALE  2.0f  // each tile is drawn at this many times TILE_SIZE

// ---------------------------------------------------------------------------
// Tile types — shared between game and map builder
//
// TILE_NONE (0) is the "empty" sentinel used in the object layer to mean
// "nothing here".  Ground cells should never contain TILE_NONE.
// ---------------------------------------------------------------------------

typedef enum {
    TILE_NONE       = 0,   // empty / transparent (object layer only)
    // Ground (walkable)
    TILE_GRASS      = 1,
    TILE_GRASS_ALT  = 2,
    TILE_PATH       = 3,
    TILE_DEAD_GRASS = 4,   // Ground/DeadGrass.png  48×32 — 6 variants
    // Terrain (not walkable)
    TILE_WATER      = 5,
    TILE_TREE       = 6,
    TILE_PINE_TREE  = 7,
    TILE_ROCK       = 8,
    TILE_WHEATFIELD = 9,   // Nature/Wheatfield.png 64×16 — 4 variants (walkable, resource)
    // Buildings — each carries an economic role
    TILE_HUT        = 10,  // simple dwelling: agents rest here
    TILE_HOUSE      = 11,  // dwelling: agents rest here
    TILE_MARKET     = 12,  // trading location
    TILE_WORKSHOP   = 13,  // production (wood → chair)
    TILE_RESOURCE   = 14,  // resource gathering / stockpile
    TILE_TAVERN     = 15,  // leisure location
    // Miscellaneous
    TILE_BRIDGE     = 16,  // Miscellaneous/Bridge.png  80×48 — 15 variants (walkable)
    TILE_WELL       = 17,  // Miscellaneous/Well.png    48×32 — 6 variants
    TILE_CHEST      = 18,  // Miscellaneous/Chests.png  32×16 — 2 variants
    TILE_SIGN       = 19,  // Miscellaneous/Signs.png   64×16 — 4 variants (walkable)
    TILE_COUNT      = 20
} TileType;

// Static metadata for each tile type
typedef struct {
    const char *label;
    bool        walkable;
    bool        isBuilding;
    bool        isHome;
    bool        isMarket;
    bool        isWorkshop;
    bool        isResource;
    bool        isLeisure;
} TileInfo;

extern const TileInfo TILE_INFO[TILE_COUNT];

// ---------------------------------------------------------------------------
// Layer identifiers
// ---------------------------------------------------------------------------
typedef enum {
    LAYER_GROUND  = 0,   // base terrain — always filled
    LAYER_OBJECTS = 1,   // buildings, trees, props — TILE_NONE means empty
} MapLayer;

// ---------------------------------------------------------------------------
// Map cells
// ---------------------------------------------------------------------------
typedef struct {
    uint8_t type;     // TileType
    uint8_t variant;  // visual variant index
} MapCell;

// ---------------------------------------------------------------------------
// Two-layer world map
//
// ground  — every cell always holds a ground tile (grass, path, water …)
// objects — every cell either holds an object tile or TILE_NONE (empty)
// ---------------------------------------------------------------------------
typedef struct {
    int      width, height;
    MapCell *ground;    // ground layer  (row-major: [y*width + x])
    MapCell *objects;   // object layer  (TILE_NONE type == empty)
} WorldMap;

// ---------------------------------------------------------------------------
// Map lifecycle and I/O
//
// Binary format v2:
//   [4]  magic  "ECON"
//   [2]  version uint16 = 2
//   [2]  width   uint16
//   [2]  height  uint16
//   [w×h × 2]  ground cells (MapCell)
//   [w×h × 2]  object cells (MapCell)
//
// v1 maps (single cell array) are loaded into ground; objects layer is empty.
// ---------------------------------------------------------------------------

WorldMap *worldmap_create(int width, int height);    // ground=grass v2, objects=TILE_NONE
void      worldmap_free(WorldMap *m);
bool      worldmap_save(const WorldMap *m, const char *path);
WorldMap *worldmap_load(const char *path);           // NULL on failure

// Cell accessors — return NULL if coordinates are out of bounds
MapCell *worldmap_cell(const WorldMap *m, int x, int y);      // ground layer
MapCell *worldmap_obj_cell(const WorldMap *m, int x, int y);  // object layer

#endif
