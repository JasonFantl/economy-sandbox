// mapbuilder.c — standalone tile-map painter
// Controls:
//   Left-click (map)     paint selected tile
//   Right-click (map)    erase to grass
//   Left-click (palette) select tile type
//   Arrow keys / WASD    scroll map
//   Mouse wheel          zoom in/out
//   +/-                  increase/decrease variant
//   Ctrl+S               save to DEFAULT_MAP_FILE
//   Ctrl+O               load from DEFAULT_MAP_FILE
//   Ctrl+N               new map (prompts nothing — resets to 80×60 grass)
//   Ctrl+Z               undo last stroke (single-level)

#include "raylib.h"
#include "src/world.h"
#include "src/tileset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MAP_FILE  "map.emap"
#define DEFAULT_MAP_W     80
#define DEFAULT_MAP_H     60
#define PALETTE_W         180
#define STATUS_H          24
#define MIN_SCALE         1.0f
#define MAX_SCALE         6.0f
#define SCROLL_SPEED      8.0f

// ---------------------------------------------------------------------------
// Palette colours for each tile type (fallback / solid background tint)
// ---------------------------------------------------------------------------
static const Color TILE_PALETTE_COLOR[TILE_COUNT] = {
    [TILE_GRASS]     = {  80, 160,  60, 255 },
    [TILE_GRASS_ALT] = { 100, 180,  70, 255 },
    [TILE_PATH]      = { 190, 160, 100, 255 },
    [TILE_WATER]     = {  60, 120, 200, 255 },
    [TILE_TREE]      = {  40, 110,  40, 255 },
    [TILE_PINE_TREE] = {  30,  90,  50, 255 },
    [TILE_ROCK]      = { 130, 120, 110, 255 },
    [TILE_HUT]       = { 180, 130,  60, 255 },
    [TILE_HOUSE]     = { 200, 150,  80, 255 },
    [TILE_MARKET]    = { 220, 100,  60, 255 },
    [TILE_WORKSHOP]  = { 160,  80,  40, 255 },
    [TILE_RESOURCE]  = { 120, 180,  80, 255 },
    [TILE_TAVERN]    = { 180,  80, 120, 255 },
};

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
typedef struct {
    WorldMap  *map;
    WorldMap  *undo;          // one-level undo snapshot
    TileAtlas  atlas;
    TileType   selectedType;
    int        selectedVariant;
    float      camX, camY;   // top-left map pixel in world-space
    float      scale;         // pixels per tile pixel  (tile draws at scale*TILE_SIZE)
    bool       dirty;         // unsaved changes
    bool       painting;      // currently holding mouse button
    char       statusMsg[128];
    float      statusTimer;
} MBState;

static void save_undo(MBState *s) {
    if (s->undo) worldmap_free(s->undo);
    s->undo = worldmap_create(s->map->width, s->map->height);
    memcpy(s->undo->cells, s->map->cells,
           (size_t)(s->map->width * s->map->height) * sizeof(MapCell));
}

static void show_status(MBState *s, const char *msg) {
    strncpy(s->statusMsg, msg, sizeof(s->statusMsg) - 1);
    s->statusMsg[sizeof(s->statusMsg) - 1] = '\0';
    s->statusTimer = 3.0f;
}

// Convert screen-space mouse position to tile coordinates
static bool screen_to_tile(const MBState *s, float mx, float my,
                             int mapAreaX, int *tx, int *ty) {
    float relX = mx - (float)mapAreaX + s->camX;
    float relY = my - 0.0f          + s->camY;
    float tileDisp = s->scale * (float)TILE_SIZE;
    *tx = (int)(relX / tileDisp);
    *ty = (int)(relY / tileDisp);
    return (*tx >= 0 && *tx < s->map->width &&
            *ty >= 0 && *ty < s->map->height);
}

// ---------------------------------------------------------------------------
// Palette rendering
// ---------------------------------------------------------------------------
static void draw_palette(const MBState *s) {
    DrawRectangle(0, 0, PALETTE_W, GetScreenHeight(), (Color){20, 24, 32, 255});
    DrawRectangleLines(0, 0, PALETTE_W, GetScreenHeight(), (Color){60, 70, 90, 255});

    DrawText("Tile Palette", 8, 6, 13, WHITE);
    DrawLine(0, 22, PALETTE_W, 22, (Color){60, 70, 90, 255});

    int iy = 26;
    for (int i = 0; i < TILE_COUNT; i++) {
        bool sel = (s->selectedType == (TileType)i);
        Color bg  = sel ? (Color){60, 80, 130, 255} : (Color){30, 36, 48, 255};
        Color bdr = sel ? (Color){120, 160, 240, 255} : (Color){50, 60, 80, 255};

        DrawRectangle(4, iy, PALETTE_W - 8, 24, bg);
        DrawRectangleLines(4, iy, PALETTE_W - 8, 24, bdr);

        // Tile preview at 24×24 (scaled from 16×16)
        if (i != TILE_WATER && TILE_VARIANT_COUNT[i] > 0) {
            Rectangle src = tileatlas_src_rect((TileType)i, 0);
            Rectangle dst = { 8.0f, (float)iy + 2.0f, 20.0f, 20.0f };
            DrawTexturePro(tileatlas_texture(&s->atlas, (TileType)i), src, dst,
                           (Vector2){0,0}, 0.0f, WHITE);
        } else {
            DrawRectangle(8, iy + 2, 20, 20, TILE_PALETTE_COLOR[i]);
        }

        DrawText(TILE_INFO[i].label, 32, iy + 7, 11,
                 sel ? WHITE : (Color){170, 175, 185, 255});

        // Show economic role tag
        if (TILE_INFO[i].isMarket)    DrawText("mkt", PALETTE_W - 28, iy + 7, 9, (Color){255,180,80,200});
        else if (TILE_INFO[i].isHome) DrawText("home",PALETTE_W - 32, iy + 7, 9, (Color){100,200,255,200});
        else if (TILE_INFO[i].isWorkshop) DrawText("wksp",PALETTE_W-34,iy+7,9,(Color){200,200,100,200});
        else if (TILE_INFO[i].isResource) DrawText("res", PALETTE_W-28,iy+7,9,(Color){100,220,120,200});
        else if (TILE_INFO[i].isLeisure)  DrawText("lei", PALETTE_W-24,iy+7,9,(Color){220,120,180,200});

        iy += 28;
    }

    // Variant selector
    iy += 6;
    DrawLine(4, iy, PALETTE_W - 4, iy, (Color){60, 70, 90, 255});
    iy += 6;
    char vbuf[48];
    int vc = TILE_VARIANT_COUNT[s->selectedType];
    if (vc > 0) {
        snprintf(vbuf, sizeof(vbuf), "Variant: %d/%d  [+/-]",
                 s->selectedVariant, vc - 1);
        DrawText(vbuf, 8, iy, 11, (Color){200, 200, 200, 255});
    } else {
        DrawText("No variants", 8, iy, 11, (Color){120, 120, 120, 255});
    }
}

// ---------------------------------------------------------------------------
// Map rendering
// ---------------------------------------------------------------------------
static void draw_map(const MBState *s, int mapAreaX) {
    float tileDisp = s->scale * (float)TILE_SIZE;
    int screenW = GetScreenWidth()  - mapAreaX;
    int screenH = GetScreenHeight() - STATUS_H;

    int startX = (int)(s->camX / tileDisp);
    int startY = (int)(s->camY / tileDisp);
    int endX   = startX + (int)(screenW / tileDisp) + 2;
    int endY   = startY + (int)(screenH / tileDisp) + 2;
    if (endX > s->map->width)  endX = s->map->width;
    if (endY > s->map->height) endY = s->map->height;

    for (int ty = startY; ty < endY; ty++) {
        for (int tx = startX; tx < endX; tx++) {
            MapCell *cell = worldmap_cell(s->map, tx, ty);
            if (!cell) continue;
            int px = mapAreaX + (int)((float)tx * tileDisp - s->camX);
            int py =            (int)((float)ty * tileDisp - s->camY);
            tileatlas_draw_cell(&s->atlas, cell, px, py, s->scale);
        }
    }

    // Grid overlay (only visible at higher zoom levels)
    if (s->scale >= 2.5f) {
        Color gridCol = {255, 255, 255, 20};
        for (int ty = startY; ty <= endY; ty++) {
            int py = (int)((float)ty * tileDisp - s->camY);
            DrawLine(mapAreaX, py, GetScreenWidth(), py, gridCol);
        }
        for (int tx = startX; tx <= endX; tx++) {
            int px = mapAreaX + (int)((float)tx * tileDisp - s->camX);
            DrawLine(px, 0, px, screenH, gridCol);
        }
    }

    // Tile highlight under cursor
    Vector2 m = GetMousePosition();
    int htx, hty;
    if (screen_to_tile(s, m.x, m.y, mapAreaX, &htx, &hty)) {
        int hpx = mapAreaX + (int)((float)htx * tileDisp - s->camX);
        int hpy =            (int)((float)hty * tileDisp - s->camY);
        int sz  = (int)tileDisp;
        DrawRectangleLines(hpx, hpy, sz, sz, (Color){255, 255, 255, 160});
        // Show type name
        char tbuf[48];
        MapCell *hc = worldmap_cell(s->map, htx, hty);
        if (hc) snprintf(tbuf, sizeof(tbuf), "[%d,%d] %s v%d",
                         htx, hty, TILE_INFO[hc->type].label, hc->variant);
        DrawText(tbuf, mapAreaX + 4, 4, 11, (Color){255,255,255,200});
    }
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------
static void draw_status(const MBState *s) {
    int sy = GetScreenHeight() - STATUS_H;
    DrawRectangle(0, sy, GetScreenWidth(), STATUS_H, (Color){15, 18, 28, 255});
    DrawLine(0, sy, GetScreenWidth(), sy, (Color){60, 70, 90, 255});

    char buf[256];
    snprintf(buf, sizeof(buf),
             "  %s  |  %dx%d  |  zoom %.0fx  |  %s  |  Ctrl+S save  Ctrl+O load  Ctrl+Z undo",
             DEFAULT_MAP_FILE, s->map->width, s->map->height,
             s->scale, s->dirty ? "UNSAVED *" : "saved");
    DrawText(buf, 4, sy + 6, 11, (Color){160, 170, 185, 255});

    if (s->statusTimer > 0.0f) {
        int tw = MeasureText(s->statusMsg, 12);
        DrawText(s->statusMsg, GetScreenWidth() - tw - 12, sy + 6, 12,
                 (Color){255, 220, 80, 255});
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    const int SCREEN_W = 1200;
    const int SCREEN_H = 760;

    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox — Map Builder");
    SetTargetFPS(60);

    MBState s = {0};
    s.map           = worldmap_create(DEFAULT_MAP_W, DEFAULT_MAP_H);
    s.undo          = NULL;
    s.selectedType  = TILE_GRASS;
    s.selectedVariant = 0;
    s.scale         = 3.0f;
    s.dirty         = false;
    show_status(&s, "New map created");

    tileatlas_load(&s.atlas);

    // Try loading existing map
    WorldMap *loaded = worldmap_load(DEFAULT_MAP_FILE);
    if (loaded) {
        worldmap_free(s.map);
        s.map = loaded;
        show_status(&s, "Loaded " DEFAULT_MAP_FILE);
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int mapAreaX = PALETTE_W + 4;
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        // --- Camera scroll ---
        float spd = SCROLL_SPEED * s.scale;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) s.camX += spd;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) s.camX -= spd;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) s.camY += spd;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) s.camY -= spd;
        // Clamp camera
        float maxCamX = (float)(s.map->width  * TILE_SIZE) * s.scale - (float)(SCREEN_W - mapAreaX);
        float maxCamY = (float)(s.map->height * TILE_SIZE) * s.scale - (float)(SCREEN_H - STATUS_H);
        if (s.camX < 0) s.camX = 0;
        if (s.camY < 0) s.camY = 0;
        if (maxCamX > 0 && s.camX > maxCamX) s.camX = maxCamX;
        if (maxCamY > 0 && s.camY > maxCamY) s.camY = maxCamY;

        // --- Zoom (mouse wheel) ---
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            Vector2 m = GetMousePosition();
            if (m.x > mapAreaX) {
                float oldScale = s.scale;
                s.scale *= (wheel > 0 ? 1.15f : 0.87f);
                if (s.scale < MIN_SCALE) s.scale = MIN_SCALE;
                if (s.scale > MAX_SCALE) s.scale = MAX_SCALE;
                // Zoom toward cursor
                float ratio = s.scale / oldScale;
                s.camX = ratio * (s.camX + (m.x - mapAreaX)) - (m.x - mapAreaX);
                s.camY = ratio * (s.camY + m.y)              -  m.y;
                if (s.camX < 0) s.camX = 0;
                if (s.camY < 0) s.camY = 0;
            }
        }

        // --- Keyboard zoom ---
        if (!ctrl && IsKeyPressed(KEY_EQUAL)) { s.scale = (s.scale < MAX_SCALE) ? s.scale + 0.5f : s.scale; }
        if (!ctrl && IsKeyPressed(KEY_MINUS)) { s.scale = (s.scale > MIN_SCALE) ? s.scale - 0.5f : s.scale; }

        // --- Variant change ---
        if (IsKeyPressed(KEY_EQUAL) && ctrl) {
            int vc = TILE_VARIANT_COUNT[s.selectedType];
            if (vc > 0) s.selectedVariant = (s.selectedVariant + 1) % vc;
        }
        if (IsKeyPressed(KEY_MINUS) && ctrl) {
            int vc = TILE_VARIANT_COUNT[s.selectedType];
            if (vc > 0) s.selectedVariant = (s.selectedVariant + vc - 1) % vc;
        }
        if (IsKeyPressed(KEY_KP_ADD)) {
            int vc = TILE_VARIANT_COUNT[s.selectedType];
            if (vc > 0) s.selectedVariant = (s.selectedVariant + 1) % vc;
        }
        if (IsKeyPressed(KEY_KP_SUBTRACT)) {
            int vc = TILE_VARIANT_COUNT[s.selectedType];
            if (vc > 0) s.selectedVariant = (s.selectedVariant + vc - 1) % vc;
        }

        // --- Save / load / new / undo ---
        if (ctrl && IsKeyPressed(KEY_S)) {
            if (worldmap_save(s.map, DEFAULT_MAP_FILE)) {
                s.dirty = false;
                show_status(&s, "Saved to " DEFAULT_MAP_FILE);
            } else {
                show_status(&s, "ERROR: save failed");
            }
        }
        if (ctrl && IsKeyPressed(KEY_O)) {
            WorldMap *m2 = worldmap_load(DEFAULT_MAP_FILE);
            if (m2) {
                worldmap_free(s.map);
                s.map   = m2;
                s.dirty = false;
                show_status(&s, "Loaded " DEFAULT_MAP_FILE);
            } else {
                show_status(&s, "ERROR: load failed (file missing or corrupt)");
            }
        }
        if (ctrl && IsKeyPressed(KEY_N)) {
            save_undo(&s);
            worldmap_free(s.map);
            s.map   = worldmap_create(DEFAULT_MAP_W, DEFAULT_MAP_H);
            s.dirty = true;
            show_status(&s, "New map");
        }
        if (ctrl && IsKeyPressed(KEY_Z)) {
            if (s.undo) {
                WorldMap *tmp = s.map;
                s.map  = s.undo;
                s.undo = tmp;
                s.dirty = true;
                show_status(&s, "Undo");
            }
        }

        // --- Palette click ---
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x < PALETTE_W) {
            int iy = 26;
            for (int i = 0; i < TILE_COUNT; i++) {
                if (mouse.y >= iy && mouse.y < iy + 24) {
                    s.selectedType    = (TileType)i;
                    s.selectedVariant = 0;
                    break;
                }
                iy += 28;
            }
        }

        // --- Map painting ---
        bool leftDown  = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        bool rightDown = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

        if ((leftDown || rightDown) && mouse.x > mapAreaX && mouse.y < SCREEN_H - STATUS_H) {
            if (!s.painting) {
                save_undo(&s);
                s.painting = true;
            }
            int tx, ty;
            if (screen_to_tile(&s, mouse.x, mouse.y, mapAreaX, &tx, &ty)) {
                MapCell *cell = worldmap_cell(s.map, tx, ty);
                if (cell) {
                    if (leftDown) {
                        cell->type    = (uint8_t)s.selectedType;
                        cell->variant = (uint8_t)s.selectedVariant;
                    } else {
                        cell->type    = TILE_GRASS;
                        cell->variant = 0;
                    }
                    s.dirty = true;
                }
            }
        } else {
            s.painting = false;
        }

        // --- Status timer ---
        if (s.statusTimer > 0.0f) s.statusTimer -= dt;

        // --- Draw ---
        BeginDrawing();
        ClearBackground((Color){10, 12, 18, 255});

        draw_map(&s, mapAreaX);
        draw_palette(&s);
        draw_status(&s);

        // Palette separator line
        DrawLine(PALETTE_W + 2, 0, PALETTE_W + 2, SCREEN_H, (Color){60, 70, 90, 255});

        DrawFPS(mapAreaX + 8, SCREEN_H - STATUS_H - 16);

        EndDrawing();
    }

    tileatlas_unload(&s.atlas);
    worldmap_free(s.map);
    if (s.undo) worldmap_free(s.undo);
    CloseWindow();
    return 0;
}
