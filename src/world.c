#include "world.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Static tile metadata table
// ---------------------------------------------------------------------------
//                              label             walk   bldg   home   mkt    wkshp  rsrc   leis
const TileInfo TILE_INFO[TILE_COUNT] = {
    [TILE_GRASS]      = { "Grass",         true,  false, false, false, false, false, false },
    [TILE_GRASS_ALT]  = { "Grass (alt)",   true,  false, false, false, false, false, false },
    [TILE_PATH]       = { "Path",          true,  false, false, false, false, false, false },
    [TILE_DEAD_GRASS] = { "Dead Grass",    true,  false, false, false, false, false, false },
    [TILE_WATER]      = { "Water",         false, false, false, false, false, false, false },
    [TILE_TREE]       = { "Tree",          false, false, false, false, false, true,  false },
    [TILE_PINE_TREE]  = { "Pine Tree",     false, false, false, false, false, false, false },
    [TILE_ROCK]       = { "Rock",          false, false, false, false, false, false, false },
    [TILE_WHEATFIELD] = { "Wheatfield",    true,  false, false, false, false, true,  false },
    [TILE_HUT]        = { "Hut",           false, true,  true,  false, false, false, false },
    [TILE_HOUSE]      = { "House",         false, true,  true,  false, false, false, false },
    [TILE_MARKET]     = { "Market",        false, true,  false, true,  false, false, false },
    [TILE_WORKSHOP]   = { "Workshop",      false, true,  false, false, true,  false, false },
    [TILE_RESOURCE]   = { "Resource Store",false, true,  false, false, false, true,  false },
    [TILE_TAVERN]     = { "Tavern",        false, true,  false, false, false, false, true  },
    [TILE_BRIDGE]     = { "Bridge",        true,  false, false, false, false, false, false },
    [TILE_WELL]       = { "Well",          false, false, false, false, false, false, false },
    [TILE_CHEST]      = { "Chest",         false, false, false, false, false, true,  false },
    [TILE_SIGN]       = { "Sign",          true,  false, false, false, false, false, false },
};

// ---------------------------------------------------------------------------
// WorldMap lifecycle
// ---------------------------------------------------------------------------

WorldMap *worldmap_create(int width, int height) {
    WorldMap *m = malloc(sizeof(WorldMap));
    if (!m) return NULL;
    m->width  = width;
    m->height = height;
    m->cells  = malloc((size_t)(width * height) * sizeof(MapCell));
    if (!m->cells) { free(m); return NULL; }
    // Fill with middle grass variant (variant 2 = mid-green)
    for (int i = 0; i < width * height; i++) {
        m->cells[i].type    = TILE_GRASS;
        m->cells[i].variant = 2;
    }
    return m;
}

void worldmap_free(WorldMap *m) {
    if (!m) return;
    free(m->cells);
    free(m);
}

MapCell *worldmap_cell(const WorldMap *m, int x, int y) {
    if (!m || x < 0 || x >= m->width || y < 0 || y >= m->height) return NULL;
    return &m->cells[y * m->width + x];
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------

bool worldmap_save(const WorldMap *m, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    const uint8_t magic[4] = {'E','C','O','N'};
    uint16_t ver = 1;
    uint16_t w   = (uint16_t)m->width;
    uint16_t h   = (uint16_t)m->height;

    fwrite(magic, 1, 4, f);
    fwrite(&ver,  2, 1, f);
    fwrite(&w,    2, 1, f);
    fwrite(&h,    2, 1, f);
    fwrite(m->cells, sizeof(MapCell), (size_t)(m->width * m->height), f);

    fclose(f);
    return true;
}

WorldMap *worldmap_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    uint8_t magic[4];
    fread(magic, 1, 4, f);
    if (magic[0]!='E' || magic[1]!='C' || magic[2]!='O' || magic[3]!='N') {
        fclose(f); return NULL;
    }

    uint16_t ver, w, h;
    fread(&ver, 2, 1, f);
    fread(&w,   2, 1, f);
    fread(&h,   2, 1, f);

    WorldMap *m = worldmap_create((int)w, (int)h);
    if (!m) { fclose(f); return NULL; }

    fread(m->cells, sizeof(MapCell), (size_t)(w * h), f);
    fclose(f);
    return m;
}
