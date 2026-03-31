#ifndef TILESET_H
#define TILESET_H

#include "world.h"
#include "raylib.h"

// ---------------------------------------------------------------------------
// How many visual variants each tile type has.
// Variant index wraps automatically when drawing.
// ---------------------------------------------------------------------------
extern const int TILE_VARIANT_COUNT[TILE_COUNT];

// ---------------------------------------------------------------------------
// TileAtlas — all tile textures loaded into GPU memory.
// Call tileatlas_load() once at startup, tileatlas_unload() at shutdown.
// Both the game and the map builder use the same functions.
// ---------------------------------------------------------------------------
typedef struct {
    Texture2D grass;       // 80×16  — 5 variants (TILE_GRASS)
    Texture2D grass_alt;   // 48×32  — 6 variants (TILE_GRASS_ALT)
    Texture2D path;        // 80×16  — 5 variants (TILE_PATH)
    Texture2D tree;        // 64×16  — 4 variants (TILE_TREE)
    Texture2D pine;        // 48×16  — 3 variants (TILE_PINE_TREE)
    Texture2D rock;        // 48×64  — 12 variants in 3×4 grid (TILE_ROCK)
    Texture2D hut;         // 80×16  — 5 variants (TILE_HUT)
    Texture2D house;       // 48×64  — 12 variants in 3×4 grid (TILE_HOUSE)
    Texture2D market;      // 48×64  — 12 variants in 3×4 grid (TILE_MARKET)
    Texture2D workshop;    // 48×48  — 9 variants in 3×3 grid (TILE_WORKSHOP)
    Texture2D resource;    // 48×80  — 15 variants in 3×5 grid (TILE_RESOURCE)
    Texture2D tavern;      // 48×64  — 12 variants in 3×4 grid (TILE_TAVERN)
} TileAtlas;

void tileatlas_load(TileAtlas *a);
void tileatlas_unload(TileAtlas *a);

// Draw one tile at pixel position (px, py) with the given scale factor.
// variant is clamped to [0, TILE_VARIANT_COUNT[type]-1].
void tileatlas_draw(const TileAtlas *a, TileType type, int variant,
                    int px, int py, float scale);

// Draw one tile from a MapCell at pixel position.
void tileatlas_draw_cell(const TileAtlas *a, const MapCell *cell,
                         int px, int py, float scale);

// Return the source Rectangle for a given tile type and variant.
// Useful for palette previews.
Rectangle tileatlas_src_rect(TileType type, int variant);
Texture2D tileatlas_texture(const TileAtlas *a, TileType type);

#endif
