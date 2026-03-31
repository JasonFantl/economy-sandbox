#include "world.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Static tile metadata table
// ---------------------------------------------------------------------------
//                              label             walk   bldg   home   mkt    wkshp  rsrc   leis
const TileInfo TILE_INFO[TILE_COUNT] = {
    [TILE_NONE]       = { "(empty)",       false, false, false, false, false, false, false },
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

    int n = width * height;
    m->ground  = malloc((size_t)n * sizeof(MapCell));
    m->objects = malloc((size_t)n * sizeof(MapCell));
    if (!m->ground || !m->objects) {
        free(m->ground); free(m->objects); free(m); return NULL;
    }

    // Ground: fill with mid-green grass (variant 2)
    for (int i = 0; i < n; i++) {
        m->ground[i].type    = TILE_GRASS;
        m->ground[i].variant = 2;
    }
    // Objects: all empty
    for (int i = 0; i < n; i++) {
        m->objects[i].type    = TILE_NONE;
        m->objects[i].variant = 0;
    }
    return m;
}

void worldmap_free(WorldMap *m) {
    if (!m) return;
    free(m->ground);
    free(m->objects);
    free(m);
}

MapCell *worldmap_cell(const WorldMap *m, int x, int y) {
    if (!m || x < 0 || x >= m->width || y < 0 || y >= m->height) return NULL;
    return &m->ground[y * m->width + x];
}

MapCell *worldmap_obj_cell(const WorldMap *m, int x, int y) {
    if (!m || x < 0 || x >= m->width || y < 0 || y >= m->height) return NULL;
    return &m->objects[y * m->width + x];
}

// ---------------------------------------------------------------------------
// File I/O
//
// v2: magic "ECON", version=2, width, height, ground cells, object cells
// v1: magic "ECON", version=1, width, height, single cells array
//     → load into ground; create empty objects layer
// ---------------------------------------------------------------------------

bool worldmap_save(const WorldMap *m, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    const uint8_t magic[4] = {'E', 'C', 'O', 'N'};
    uint16_t ver = 2;
    uint16_t w   = (uint16_t)m->width;
    uint16_t h   = (uint16_t)m->height;
    int      n   = m->width * m->height;

    fwrite(magic,    1, 4, f);
    fwrite(&ver,     2, 1, f);
    fwrite(&w,       2, 1, f);
    fwrite(&h,       2, 1, f);
    fwrite(m->ground,  sizeof(MapCell), (size_t)n, f);
    fwrite(m->objects, sizeof(MapCell), (size_t)n, f);

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

    int n = (int)w * (int)h;

    if (ver == 2) {
        fread(m->ground,  sizeof(MapCell), (size_t)n, f);
        fread(m->objects, sizeof(MapCell), (size_t)n, f);
    } else {
        // v1: single layer — load into ground, objects stay empty (TILE_NONE)
        fread(m->ground, sizeof(MapCell), (size_t)n, f);
    }

    fclose(f);
    return m;
}
