#include "tileset.h"
#include <stdio.h>

// ---------------------------------------------------------------------------
// Variant counts per tile type
// ---------------------------------------------------------------------------

const int TILE_VARIANT_COUNT[TILE_COUNT] = {
    [TILE_NONE]       = 0,   // empty — no texture
    // Ground
    [TILE_GRASS]      = 5,   // 80×16  strip
    [TILE_GRASS_ALT]  = 6,   // 48×32  3×2 grid
    [TILE_PATH]       = 5,   // 80×16  strip
    [TILE_DEAD_GRASS] = 6,   // 48×32  3×2 grid
    // Terrain
    [TILE_WATER]      = 0,   // drawn as solid colour
    [TILE_TREE]       = 4,   // 64×16  strip
    [TILE_PINE_TREE]  = 3,   // 48×16  strip
    [TILE_ROCK]       = 12,  // 48×64  3×4 grid
    [TILE_WHEATFIELD] = 4,   // 64×16  strip
    // Buildings
    [TILE_HUT]        = 5,   // 80×16  strip
    [TILE_HOUSE]      = 12,  // 48×64  3×4 grid
    [TILE_MARKET]     = 12,  // 48×64  3×4 grid
    [TILE_WORKSHOP]   = 9,   // 48×48  3×3 grid
    [TILE_RESOURCE]   = 15,  // 48×80  3×5 grid
    [TILE_TAVERN]     = 12,  // 48×64  3×4 grid
    // Miscellaneous
    [TILE_BRIDGE]     = 15,  // 80×48  5×3 grid
    [TILE_WELL]       = 6,   // 48×32  3×2 grid
    [TILE_CHEST]      = 2,   // 32×16  strip
    [TILE_SIGN]       = 4,   // 64×16  strip
};

// ---------------------------------------------------------------------------
// Sprite sheet layout helpers
// ---------------------------------------------------------------------------

// Horizontal strip (1 row) of 16×16 tiles — column = variant
static Rectangle strip_src(int variant) {
    return (Rectangle){ (float)(variant * TILE_SIZE), 0.0f,
                        (float)TILE_SIZE, (float)TILE_SIZE };
}

// Grid of 16×16 tiles with `cols` columns — row-major
static Rectangle grid_src(int variant, int cols) {
    int col = variant % cols;
    int row = variant / cols;
    return (Rectangle){ (float)(col * TILE_SIZE), (float)(row * TILE_SIZE),
                        (float)TILE_SIZE, (float)TILE_SIZE };
}

// ---------------------------------------------------------------------------
// Source rectangle for a tile type + variant
// ---------------------------------------------------------------------------

Rectangle tileatlas_src_rect(TileType type, int variant) {
    if (type == TILE_NONE) return (Rectangle){0, 0, 0, 0};
    int vc = TILE_VARIANT_COUNT[type];
    if (vc > 0 && variant >= vc) variant = variant % vc;

    switch (type) {
        // Horizontal strips
        case TILE_GRASS:
        case TILE_PATH:
        case TILE_PINE_TREE:
        case TILE_HUT:
        case TILE_TREE:
        case TILE_WHEATFIELD:
        case TILE_CHEST:
        case TILE_SIGN:
            return strip_src(variant);

        // 3×2 grids
        case TILE_GRASS_ALT:
        case TILE_DEAD_GRASS:
        case TILE_WELL:
            return grid_src(variant, 3);

        // 3-column grids (various heights)
        case TILE_ROCK:
        case TILE_HOUSE:
        case TILE_MARKET:
        case TILE_TAVERN:
        case TILE_WORKSHOP:
        case TILE_RESOURCE:
            return grid_src(variant, 3);

        // 5-column grid
        case TILE_BRIDGE:
            return grid_src(variant, 5);

        default:
            return (Rectangle){0, 0, TILE_SIZE, TILE_SIZE};
    }
}

// ---------------------------------------------------------------------------
// Atlas load / unload
// ---------------------------------------------------------------------------

#define ASSET_BASE "src/assets/MiniWorldSprites/"

void tileatlas_load(TileAtlas *a) {
    // Ground
    a->grass      = LoadTexture(ASSET_BASE "Ground/Grass.png");
    a->grass_alt  = LoadTexture(ASSET_BASE "Ground/TexturedGrass.png");
    a->path       = LoadTexture(ASSET_BASE "Ground/Shore.png");
    a->dead_grass = LoadTexture(ASSET_BASE "Ground/DeadGrass.png");
    // Nature
    a->tree       = LoadTexture(ASSET_BASE "Nature/Trees.png");
    a->pine       = LoadTexture(ASSET_BASE "Nature/PineTrees.png");
    a->rock       = LoadTexture(ASSET_BASE "Nature/Rocks.png");
    a->wheatfield = LoadTexture(ASSET_BASE "Nature/Wheatfield.png");
    // Buildings
    a->hut        = LoadTexture(ASSET_BASE "Buildings/Wood/Huts.png");
    a->house      = LoadTexture(ASSET_BASE "Buildings/Wood/Houses.png");
    a->market     = LoadTexture(ASSET_BASE "Buildings/Wood/Market.png");
    a->workshop   = LoadTexture(ASSET_BASE "Buildings/Wood/Workshops.png");
    a->resource   = LoadTexture(ASSET_BASE "Buildings/Wood/Resources.png");
    a->tavern     = LoadTexture(ASSET_BASE "Buildings/Wood/Taverns.png");
    // Miscellaneous
    a->bridge     = LoadTexture(ASSET_BASE "Miscellaneous/Bridge.png");
    a->well       = LoadTexture(ASSET_BASE "Miscellaneous/Well.png");
    a->chest      = LoadTexture(ASSET_BASE "Miscellaneous/Chests.png");
    a->sign       = LoadTexture(ASSET_BASE "Miscellaneous/Signs.png");

    // Nearest-neighbour for pixel art
    SetTextureFilter(a->grass,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->grass_alt,  TEXTURE_FILTER_POINT);
    SetTextureFilter(a->path,       TEXTURE_FILTER_POINT);
    SetTextureFilter(a->dead_grass, TEXTURE_FILTER_POINT);
    SetTextureFilter(a->tree,       TEXTURE_FILTER_POINT);
    SetTextureFilter(a->pine,       TEXTURE_FILTER_POINT);
    SetTextureFilter(a->rock,       TEXTURE_FILTER_POINT);
    SetTextureFilter(a->wheatfield, TEXTURE_FILTER_POINT);
    SetTextureFilter(a->hut,        TEXTURE_FILTER_POINT);
    SetTextureFilter(a->house,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->market,     TEXTURE_FILTER_POINT);
    SetTextureFilter(a->workshop,   TEXTURE_FILTER_POINT);
    SetTextureFilter(a->resource,   TEXTURE_FILTER_POINT);
    SetTextureFilter(a->tavern,     TEXTURE_FILTER_POINT);
    SetTextureFilter(a->bridge,     TEXTURE_FILTER_POINT);
    SetTextureFilter(a->well,       TEXTURE_FILTER_POINT);
    SetTextureFilter(a->chest,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->sign,       TEXTURE_FILTER_POINT);
}

void tileatlas_unload(TileAtlas *a) {
    UnloadTexture(a->grass);
    UnloadTexture(a->grass_alt);
    UnloadTexture(a->path);
    UnloadTexture(a->dead_grass);
    UnloadTexture(a->tree);
    UnloadTexture(a->pine);
    UnloadTexture(a->rock);
    UnloadTexture(a->wheatfield);
    UnloadTexture(a->hut);
    UnloadTexture(a->house);
    UnloadTexture(a->market);
    UnloadTexture(a->workshop);
    UnloadTexture(a->resource);
    UnloadTexture(a->tavern);
    UnloadTexture(a->bridge);
    UnloadTexture(a->well);
    UnloadTexture(a->chest);
    UnloadTexture(a->sign);
}

// ---------------------------------------------------------------------------
// Texture lookup
// ---------------------------------------------------------------------------

Texture2D tileatlas_texture(const TileAtlas *a, TileType type) {
    switch (type) {
        case TILE_NONE:       return a->grass;  // fallback; draw should skip TILE_NONE
        case TILE_GRASS:      return a->grass;
        case TILE_GRASS_ALT:  return a->grass_alt;
        case TILE_PATH:       return a->path;
        case TILE_DEAD_GRASS: return a->dead_grass;
        case TILE_TREE:       return a->tree;
        case TILE_PINE_TREE:  return a->pine;
        case TILE_ROCK:       return a->rock;
        case TILE_WHEATFIELD: return a->wheatfield;
        case TILE_HUT:        return a->hut;
        case TILE_HOUSE:      return a->house;
        case TILE_MARKET:     return a->market;
        case TILE_WORKSHOP:   return a->workshop;
        case TILE_RESOURCE:   return a->resource;
        case TILE_TAVERN:     return a->tavern;
        case TILE_BRIDGE:     return a->bridge;
        case TILE_WELL:       return a->well;
        case TILE_CHEST:      return a->chest;
        case TILE_SIGN:       return a->sign;
        default:              return a->grass;
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void tileatlas_draw(const TileAtlas *a, TileType type, int variant,
                    int px, int py, float scale) {
    if (type == TILE_NONE) return;   // nothing to draw on object layer

    int size = (int)(TILE_SIZE * scale);

    if (type == TILE_WATER) {
        DrawRectangle(px, py, size, size, (Color){60, 120, 200, 255});
        return;
    }

    int vc = TILE_VARIANT_COUNT[type];
    if (vc <= 0) {
        DrawRectangle(px, py, size, size, (Color){80, 80, 80, 255});
        return;
    }
    if (variant >= vc) variant = variant % vc;

    Rectangle src = tileatlas_src_rect(type, variant);
    Rectangle dst = { (float)px, (float)py, (float)size, (float)size };
    DrawTexturePro(tileatlas_texture(a, type), src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

void tileatlas_draw_cell(const TileAtlas *a, const MapCell *cell,
                         int px, int py, float scale) {
    tileatlas_draw(a, (TileType)cell->type, (int)cell->variant, px, py, scale);
}
