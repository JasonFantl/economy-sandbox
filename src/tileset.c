#include "tileset.h"
#include <stdio.h>

// ---------------------------------------------------------------------------
// Variant counts per tile type
// ---------------------------------------------------------------------------

// For tiles packed in a horizontal strip: cols × 1 row
// For tiles packed in a grid: cols × rows (computed: w/16 * h/16)
const int TILE_VARIANT_COUNT[TILE_COUNT] = {
    [TILE_GRASS]     = 5,   // 80×16  strip
    [TILE_GRASS_ALT] = 6,   // 48×32  3×2 grid
    [TILE_PATH]      = 5,   // 80×16  strip
    [TILE_WATER]     = 0,   // drawn as solid colour, no texture
    [TILE_TREE]      = 4,   // 64×16  strip
    [TILE_PINE_TREE] = 3,   // 48×16  strip
    [TILE_ROCK]      = 12,  // 48×64  3×4 grid
    [TILE_HUT]       = 5,   // 80×16  strip
    [TILE_HOUSE]     = 12,  // 48×64  3×4 grid
    [TILE_MARKET]    = 12,  // 48×64  3×4 grid
    [TILE_WORKSHOP]  = 9,   // 48×48  3×3 grid
    [TILE_RESOURCE]  = 15,  // 48×80  3×5 grid
    [TILE_TAVERN]    = 12,  // 48×64  3×4 grid
};

// ---------------------------------------------------------------------------
// Sprite sheet layout helpers
// ---------------------------------------------------------------------------

// For a strip (1 row) of 16×16 tiles: column = variant
static Rectangle strip_src(int variant) {
    return (Rectangle){ (float)(variant * TILE_SIZE), 0.0f,
                        (float)TILE_SIZE, (float)TILE_SIZE };
}

// For a grid of 16×16 tiles with `cols` columns: row-major layout
static Rectangle grid_src(int variant, int cols) {
    int col = variant % cols;
    int row = variant / cols;
    return (Rectangle){ (float)(col * TILE_SIZE), (float)(row * TILE_SIZE),
                        (float)TILE_SIZE, (float)TILE_SIZE };
}

// ---------------------------------------------------------------------------
// Source rectangle for a given tile type + variant
// (No atlas pointer needed — layout is determined by type alone.)
// ---------------------------------------------------------------------------

Rectangle tileatlas_src_rect(TileType type, int variant) {
    int vc = TILE_VARIANT_COUNT[type];
    if (vc > 0 && variant >= vc) variant = variant % vc;

    switch (type) {
        // Horizontal strips (1 row)
        case TILE_GRASS:
        case TILE_PATH:
        case TILE_PINE_TREE:
        case TILE_HUT:
            return strip_src(variant);

        // Grass alt: 3×2 grid
        case TILE_GRASS_ALT:
            return grid_src(variant, 3);

        // Trees: horizontal strip
        case TILE_TREE:
            return strip_src(variant);

        // Grids (3 columns each)
        case TILE_ROCK:
        case TILE_HOUSE:
        case TILE_MARKET:
        case TILE_TAVERN:
            return grid_src(variant, 3);

        case TILE_WORKSHOP:
            return grid_src(variant, 3);

        case TILE_RESOURCE:
            return grid_src(variant, 3);

        default:
            return (Rectangle){0, 0, TILE_SIZE, TILE_SIZE};
    }
}

// ---------------------------------------------------------------------------
// Atlas load / unload
// ---------------------------------------------------------------------------

#define ASSET_BASE "src/assets/MiniWorldSprites/"

void tileatlas_load(TileAtlas *a) {
    a->grass    = LoadTexture(ASSET_BASE "Ground/Grass.png");
    a->grass_alt= LoadTexture(ASSET_BASE "Ground/TexturedGrass.png");
    a->path     = LoadTexture(ASSET_BASE "Ground/Shore.png");
    a->tree     = LoadTexture(ASSET_BASE "Nature/Trees.png");
    a->pine     = LoadTexture(ASSET_BASE "Nature/PineTrees.png");
    a->rock     = LoadTexture(ASSET_BASE "Nature/Rocks.png");
    a->hut      = LoadTexture(ASSET_BASE "Buildings/Wood/Huts.png");
    a->house    = LoadTexture(ASSET_BASE "Buildings/Wood/Houses.png");
    a->market   = LoadTexture(ASSET_BASE "Buildings/Wood/Market.png");
    a->workshop = LoadTexture(ASSET_BASE "Buildings/Wood/Workshops.png");
    a->resource = LoadTexture(ASSET_BASE "Buildings/Wood/Resources.png");
    a->tavern   = LoadTexture(ASSET_BASE "Buildings/Wood/Taverns.png");

    // Use nearest-neighbour filtering for pixel art
    SetTextureFilter(a->grass,     TEXTURE_FILTER_POINT);
    SetTextureFilter(a->grass_alt, TEXTURE_FILTER_POINT);
    SetTextureFilter(a->path,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->tree,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->pine,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->rock,      TEXTURE_FILTER_POINT);
    SetTextureFilter(a->hut,       TEXTURE_FILTER_POINT);
    SetTextureFilter(a->house,     TEXTURE_FILTER_POINT);
    SetTextureFilter(a->market,    TEXTURE_FILTER_POINT);
    SetTextureFilter(a->workshop,  TEXTURE_FILTER_POINT);
    SetTextureFilter(a->resource,  TEXTURE_FILTER_POINT);
    SetTextureFilter(a->tavern,    TEXTURE_FILTER_POINT);
}

void tileatlas_unload(TileAtlas *a) {
    UnloadTexture(a->grass);
    UnloadTexture(a->grass_alt);
    UnloadTexture(a->path);
    UnloadTexture(a->tree);
    UnloadTexture(a->pine);
    UnloadTexture(a->rock);
    UnloadTexture(a->hut);
    UnloadTexture(a->house);
    UnloadTexture(a->market);
    UnloadTexture(a->workshop);
    UnloadTexture(a->resource);
    UnloadTexture(a->tavern);
}

// ---------------------------------------------------------------------------
// Texture lookup
// ---------------------------------------------------------------------------

Texture2D tileatlas_texture(const TileAtlas *a, TileType type) {
    switch (type) {
        case TILE_GRASS:     return a->grass;
        case TILE_GRASS_ALT: return a->grass_alt;
        case TILE_PATH:      return a->path;
        case TILE_TREE:      return a->tree;
        case TILE_PINE_TREE: return a->pine;
        case TILE_ROCK:      return a->rock;
        case TILE_HUT:       return a->hut;
        case TILE_HOUSE:     return a->house;
        case TILE_MARKET:    return a->market;
        case TILE_WORKSHOP:  return a->workshop;
        case TILE_RESOURCE:  return a->resource;
        case TILE_TAVERN:    return a->tavern;
        default:             return a->grass;
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void tileatlas_draw(const TileAtlas *a, TileType type, int variant,
                    int px, int py, float scale) {
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
