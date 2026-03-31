// mapbuilder/mapbuilder.c — Economy Sandbox Map Builder
//
// Palette: organised by category (Ground / Nature / Buildings / Miscellaneous)
// with sprite previews, variant strip, and economic-role tags.
//
// Tools (keyboard shortcut):
//   P  Pencil     — paint tile(s), drag to paint, right-click erases to grass
//   B  Bucket     — flood-fill connected region
//   I  Eyedrop    — pick tile from map, auto-switches to Pencil
//   R  Rectangle  — drag to fill a filled rectangle
//
// Brush size (Pencil only):  [  decrease   ]  increase   (1 / 3 / 5)
// Variant:  + / -  or click the variant strip in the palette
//
// Camera:  WASD / arrow keys scroll  |  mouse-wheel zooms to cursor  |  Home centres
//
// File:  Ctrl+S save   Ctrl+O load   Ctrl+N new map (80x60)   Ctrl+Z undo

#include "raylib.h"
#include "src/world.h"
#include "src/tileset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
#define SCREEN_W       1280
#define SCREEN_H        800
#define PALETTE_W       224
#define TOOLBAR_H        46
#define STATUS_H         26
#define TILE_PREVIEW_SZ  20
#define TILE_ROW_H       23
#define CAT_HDR_H        21
#define MIN_SCALE        0.5f
#define MAX_SCALE        8.0f
#define DEFAULT_MAP_W    80
#define DEFAULT_MAP_H    60
#define DEFAULT_FILE     "map.emap"

// ---------------------------------------------------------------------------
// Palette categories
// ---------------------------------------------------------------------------
typedef struct {
    const char *label;
    Color       accent;
    TileType    tiles[12];
    int         count;
    bool        collapsed;
} PaletteGroup;

static PaletteGroup g_groups[] = {
    { "Ground",
      { 85, 172,  64, 255 },
      { TILE_GRASS, TILE_GRASS_ALT, TILE_DEAD_GRASS, TILE_PATH, TILE_WATER },
      5, false },
    { "Nature",
      { 45, 148,  48, 255 },
      { TILE_TREE, TILE_PINE_TREE, TILE_ROCK, TILE_WHEATFIELD },
      4, false },
    { "Buildings",
      {208, 142,  72, 255 },
      { TILE_HUT, TILE_HOUSE, TILE_MARKET, TILE_WORKSHOP, TILE_RESOURCE, TILE_TAVERN },
      6, false },
    { "Miscellaneous",
      {136, 162, 210, 255 },
      { TILE_BRIDGE, TILE_WELL, TILE_CHEST, TILE_SIGN },
      4, false },
};
#define GROUP_COUNT 4

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------
static const Color C_BG       = { 18,  20,  28, 255};
static const Color C_PAL_BG   = { 22,  26,  38, 255};
static const Color C_CAT_HDR  = { 30,  38,  56, 255};
static const Color C_SEL_BG   = { 45,  70, 128, 255};
static const Color C_HOV_BG   = { 36,  44,  66, 255};
static const Color C_TOOLBAR  = { 24,  28,  42, 255};
static const Color C_STATUS   = { 13,  16,  23, 255};
static const Color C_BORDER   = { 46,  56,  80, 255};
static const Color C_TEXT     = {218, 224, 236, 255};
static const Color C_TEXT_DIM = {128, 140, 160, 255};
static const Color C_SEL_RING = { 96, 156, 255, 255};
static const Color C_HOVER    = {255, 255, 255, 130};
static const Color C_RECT_FILL= {100, 160, 255,  40};
static const Color C_RECT_BDR = {100, 160, 255, 200};

// ---------------------------------------------------------------------------
// Tools
// ---------------------------------------------------------------------------
typedef enum {
    TOOL_PENCIL  = 0,
    TOOL_BUCKET  = 1,
    TOOL_EYEDROP = 2,
    TOOL_RECT    = 3,
    TOOL_COUNT   = 4,
} EditTool;

static const char *TOOL_LABEL[TOOL_COUNT] = { "Pencil", "Bucket", "Eyedrop", "Rect" };
static const char *TOOL_KEY[TOOL_COUNT]   = { "P", "B", "I", "R" };

// Toolbar button layout constants (must match draw_toolbar)
#define TB_TOOL_X0    56
#define TB_TOOL_W     62
#define TB_BTN_H      34
#define TB_BTN_INNER  58
// Brush buttons start after tools + separator + label
#define TB_BRUSH_X0   (TB_TOOL_X0 + TOOL_COUNT * TB_TOOL_W + 4 + 12 + 44)
#define TB_BRUSH_W    30
#define TB_BRUSH_BTN  28

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
typedef struct {
    WorldMap  *map;
    WorldMap  *undo;
    TileAtlas  atlas;
    TileType   selType;
    int        selVariant;
    float      camX, camY;
    float      scale;
    bool       dirty;
    bool       painting;
    EditTool   tool;
    int        brushSize;
    bool       rectActive;
    int        rectX0, rectY0, rectX1, rectY1;
    char       statusMsg[192];
    float      statusTimer;
    char       filePath[256];
} MBState;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void show_status(MBState *s, const char *msg) {
    strncpy(s->statusMsg, msg, sizeof(s->statusMsg) - 1);
    s->statusMsg[sizeof(s->statusMsg) - 1] = '\0';
    s->statusTimer = 3.5f;
}

static void save_undo(MBState *s) {
    if (s->undo) worldmap_free(s->undo);
    s->undo = worldmap_create(s->map->width, s->map->height);
    if (s->undo)
        memcpy(s->undo->cells, s->map->cells,
               (size_t)(s->map->width * s->map->height) * sizeof(MapCell));
}

static int map_ax(void) { return PALETTE_W + 2; }
static int map_ay(void) { return TOOLBAR_H; }
static int map_aw(void) { return SCREEN_W - map_ax(); }
static int map_ah(void) { return SCREEN_H - TOOLBAR_H - STATUS_H; }

static bool screen_to_tile(const MBState *s, float mx, float my, int *tx, int *ty) {
    float td = s->scale * (float)TILE_SIZE;
    *tx = (int)((mx - (float)map_ax() + s->camX) / td);
    *ty = (int)((my - (float)map_ay() + s->camY) / td);
    return (*tx >= 0 && *tx < s->map->width &&
            *ty >= 0 && *ty < s->map->height);
}

static int tile_px(const MBState *s, int tx) {
    return map_ax() + (int)((float)tx * s->scale * TILE_SIZE - s->camX);
}
static int tile_py(const MBState *s, int ty) {
    return map_ay() + (int)((float)ty * s->scale * TILE_SIZE - s->camY);
}

static void draw_tile_icon(const TileAtlas *a, TileType type, int variant,
                            int px, int py, int sz) {
    if (type == TILE_WATER) {
        DrawRectangle(px, py, sz, sz, (Color){60, 120, 200, 255});
        return;
    }
    int vc = TILE_VARIANT_COUNT[type];
    if (vc <= 0) { DrawRectangle(px, py, sz, sz, (Color){70, 70, 70, 255}); return; }
    if (variant >= vc) variant %= vc;
    Rectangle src = tileatlas_src_rect(type, variant);
    Rectangle dst = {(float)px, (float)py, (float)sz, (float)sz};
    DrawTexturePro(tileatlas_texture(a, type), src, dst, (Vector2){0,0}, 0.0f, WHITE);
}

static const char *role_tag(TileType t) {
    if (TILE_INFO[t].isMarket)   return "mkt";
    if (TILE_INFO[t].isHome)     return "home";
    if (TILE_INFO[t].isWorkshop) return "wksp";
    if (TILE_INFO[t].isResource) return "res";
    if (TILE_INFO[t].isLeisure)  return "lei";
    return NULL;
}
static Color role_color(TileType t) {
    if (TILE_INFO[t].isMarket)   return (Color){255, 190,  80, 230};
    if (TILE_INFO[t].isHome)     return (Color){100, 200, 255, 230};
    if (TILE_INFO[t].isWorkshop) return (Color){210, 210, 100, 230};
    if (TILE_INFO[t].isResource) return (Color){100, 230, 120, 230};
    if (TILE_INFO[t].isLeisure)  return (Color){240, 130, 190, 230};
    return WHITE;
}

// ---------------------------------------------------------------------------
// Flood fill (BFS)
// ---------------------------------------------------------------------------
static void flood_fill(MBState *s, int startX, int startY) {
    MapCell *origin = worldmap_cell(s->map, startX, startY);
    if (!origin) return;
    uint8_t srcType = origin->type, srcVar = origin->variant;
    if (srcType == (uint8_t)s->selType && srcVar == (uint8_t)s->selVariant) return;

    int total = s->map->width * s->map->height;
    int  *qx  = malloc(total * sizeof(int));
    int  *qy  = malloc(total * sizeof(int));
    bool *vis = calloc(total, 1);
    if (!qx || !qy || !vis) { free(qx); free(qy); free(vis); return; }

    int head = 0, tail = 0;
    qx[tail] = startX; qy[tail++] = startY;
    vis[startY * s->map->width + startX] = true;

    const int ddx[4] = {1, -1, 0, 0};
    const int ddy[4] = {0,  0, 1, -1};

    while (head < tail) {
        int x = qx[head], y = qy[head++];
        MapCell *c = worldmap_cell(s->map, x, y);
        if (!c || c->type != srcType || c->variant != srcVar) continue;
        c->type    = (uint8_t)s->selType;
        c->variant = (uint8_t)s->selVariant;
        for (int d = 0; d < 4; d++) {
            int nx = x + ddx[d], ny = y + ddy[d];
            if (nx >= 0 && nx < s->map->width &&
                ny >= 0 && ny < s->map->height &&
                !vis[ny * s->map->width + nx]) {
                vis[ny * s->map->width + nx] = true;
                qx[tail] = nx; qy[tail++] = ny;
            }
        }
    }
    free(qx); free(qy); free(vis);
}

// Paint a brushSize x brushSize patch centred at (tx, ty)
static void paint_brush(MBState *s, int tx, int ty) {
    int half = s->brushSize / 2;
    for (int dy = -half; dy <= half; dy++) {
        for (int dx = -half; dx <= half; dx++) {
            MapCell *c = worldmap_cell(s->map, tx + dx, ty + dy);
            if (c) { c->type = (uint8_t)s->selType; c->variant = (uint8_t)s->selVariant; }
        }
    }
    s->dirty = true;
}

// ---------------------------------------------------------------------------
// Draw: toolbar
// ---------------------------------------------------------------------------
static void draw_toolbar(const MBState *s) {
    DrawRectangle(0, 0, SCREEN_W, TOOLBAR_H, C_TOOLBAR);
    DrawLine(0, TOOLBAR_H - 1, SCREEN_W, TOOLBAR_H - 1, C_BORDER);

    int ix = 8;
    DrawText("Tools", ix, 16, 11, C_TEXT_DIM);
    ix = TB_TOOL_X0;

    for (int t = 0; t < TOOL_COUNT; t++) {
        bool sel = (s->tool == (EditTool)t);
        Color bg  = sel ? C_SEL_BG : (Color){34, 42, 60, 255};
        Color bdr = sel ? C_SEL_RING : C_BORDER;
        DrawRectangle(ix, 6, TB_BTN_INNER, TB_BTN_H, bg);
        DrawRectangleLines(ix, 6, TB_BTN_INNER, TB_BTN_H, bdr);
        DrawText(TOOL_KEY[t],   ix + 5,  11, 11, sel ? C_SEL_RING : C_TEXT_DIM);
        DrawText(TOOL_LABEL[t], ix + 18, 11, 11, sel ? C_TEXT : (Color){155, 165, 185, 255});
        ix += TB_TOOL_W;
    }

    // Separator
    ix += 4;
    DrawLine(ix, 8, ix, TOOLBAR_H - 8, C_BORDER);
    ix += 12;

    // Brush size
    DrawText("Brush", ix, 16, 11, C_TEXT_DIM);
    ix += 44;
    int brushOpts[3] = {1, 3, 5};
    for (int b = 0; b < 3; b++) {
        bool sel = (s->brushSize == brushOpts[b]);
        Color bg  = sel ? C_SEL_BG : (Color){34, 42, 60, 255};
        Color bdr = sel ? C_SEL_RING : C_BORDER;
        DrawRectangle(ix, 8, TB_BRUSH_BTN, TB_BTN_H - 2, bg);
        DrawRectangleLines(ix, 8, TB_BRUSH_BTN, TB_BTN_H - 2, bdr);
        char bb[4]; snprintf(bb, sizeof(bb), "%d", brushOpts[b]);
        int bw = MeasureText(bb, 12);
        DrawText(bb, ix + (TB_BRUSH_BTN - bw) / 2, 15, 12, sel ? C_TEXT : C_TEXT_DIM);
        ix += TB_BRUSH_W;
    }

    // Separator
    ix += 4;
    DrawLine(ix, 8, ix, TOOLBAR_H - 8, C_BORDER);
    ix += 12;

    // Selected tile preview
    draw_tile_icon(&s->atlas, s->selType, s->selVariant, ix, 7, 32);
    DrawRectangleLines(ix, 7, 32, 32, C_BORDER);
    ix += 38;
    DrawText(TILE_INFO[s->selType].label, ix, 10, 12, C_TEXT);
    int vc = TILE_VARIANT_COUNT[s->selType];
    char vbuf[48];
    if (vc > 0) snprintf(vbuf, sizeof(vbuf), "v%d/%d  (+/-)", s->selVariant, vc - 1);
    else        snprintf(vbuf, sizeof(vbuf), "no variants");
    DrawText(vbuf, ix, 26, 10, C_TEXT_DIM);
}

// ---------------------------------------------------------------------------
// Draw: palette
// ---------------------------------------------------------------------------
static void draw_palette(MBState *s) {
    int aY = map_ay();
    int aH = map_ah() + STATUS_H;
    DrawRectangle(0, aY, PALETTE_W, aH, C_PAL_BG);
    DrawLine(PALETTE_W + 1, aY, PALETTE_W + 1, aY + aH, C_BORDER);

    Vector2 mouse = GetMousePosition();
    int iy = aY + 4;

    for (int g = 0; g < GROUP_COUNT; g++) {
        PaletteGroup *grp = &g_groups[g];

        // Category header
        bool catHov = (mouse.x >= 0 && mouse.x < PALETTE_W &&
                       mouse.y >= iy && mouse.y < iy + CAT_HDR_H);
        Color catBg = catHov ? (Color){38, 48, 68, 255} : C_CAT_HDR;
        DrawRectangle(0, iy, PALETTE_W, CAT_HDR_H, catBg);
        DrawRectangle(0, iy, 3, CAT_HDR_H, grp->accent);

        char lbl[64];
        snprintf(lbl, sizeof(lbl), "  %s  %s", grp->collapsed ? "+" : "-", grp->label);
        DrawText(lbl, 6, iy + 4, 11, C_TEXT);

        // Count badge
        char cnt[8]; snprintf(cnt, sizeof(cnt), "%d", grp->count);
        int cw = MeasureText(cnt, 10);
        DrawRectangle(PALETTE_W - cw - 12, iy + 4, cw + 8, 13, (Color){44, 54, 78, 255});
        DrawText(cnt, PALETTE_W - cw - 8, iy + 5, 10, C_TEXT_DIM);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && catHov)
            grp->collapsed = !grp->collapsed;

        iy += CAT_HDR_H + 1;
        if (grp->collapsed) continue;

        // Tile rows
        for (int ti = 0; ti < grp->count; ti++) {
            TileType type = grp->tiles[ti];
            bool sel = (s->selType == type);
            bool hov = (!sel && mouse.x >= 3 && mouse.x < PALETTE_W - 3 &&
                        mouse.y >= iy && mouse.y < iy + TILE_ROW_H);

            Color rowBg = sel ? C_SEL_BG : (hov ? C_HOV_BG : C_PAL_BG);
            DrawRectangle(3, iy, PALETTE_W - 6, TILE_ROW_H, rowBg);
            if (sel) DrawRectangle(3, iy, 2, TILE_ROW_H, grp->accent);

            int variant = sel ? s->selVariant : 0;
            draw_tile_icon(&s->atlas, type, variant,
                           9, iy + (TILE_ROW_H - TILE_PREVIEW_SZ) / 2, TILE_PREVIEW_SZ);

            DrawText(TILE_INFO[type].label, 34, iy + 5, 11,
                     sel ? C_TEXT : (Color){170, 178, 196, 255});

            // Non-walkable dot
            if (!TILE_INFO[type].walkable)
                DrawCircle(PALETTE_W - 50, iy + TILE_ROW_H / 2, 3, (Color){200, 70, 70, 200});

            // Economic role tag
            const char *tag = role_tag(type);
            if (tag) {
                int tw = MeasureText(tag, 9);
                DrawText(tag, PALETTE_W - tw - 5, iy + 6, 9, role_color(type));
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hov) {
                s->selType    = type;
                s->selVariant = 0;
            }
            iy += TILE_ROW_H;
        }
        iy += 3;
    }

    // Variant strip at bottom
    int vsY = SCREEN_H - STATUS_H - 82;
    DrawLine(6, vsY, PALETTE_W - 6, vsY, C_BORDER);
    vsY += 5;

    DrawText("Variant", 8, vsY + 1, 11, C_TEXT_DIM);
    int vc = TILE_VARIANT_COUNT[s->selType];
    if (vc > 0) {
        char vbuf[32]; snprintf(vbuf, sizeof(vbuf), "%d / %d", s->selVariant, vc - 1);
        int vw = MeasureText(vbuf, 11);
        DrawText(vbuf, PALETTE_W - vw - 5, vsY + 1, 11, C_TEXT_DIM);
        vsY += 16;

        int iconSz = 22;
        int perRow = (PALETTE_W - 14) / (iconSz + 2);
        int vx = 7, vy = vsY;
        for (int v = 0; v < vc; v++) {
            bool vsel = (v == s->selVariant);
            if (vsel) DrawRectangle(vx - 1, vy - 1, iconSz + 2, iconSz + 2, C_SEL_RING);
            draw_tile_icon(&s->atlas, s->selType, v, vx, vy, iconSz);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                mouse.x >= vx && mouse.x < vx + iconSz &&
                mouse.y >= vy && mouse.y < vy + iconSz)
                s->selVariant = v;
            vx += iconSz + 2;
            if ((v + 1) % perRow == 0) { vx = 7; vy += iconSz + 2; }
        }
    } else {
        vsY += 16;
        DrawText("(no variants)", 8, vsY, 11, C_TEXT_DIM);
    }
}

// ---------------------------------------------------------------------------
// Draw: map area
// ---------------------------------------------------------------------------
static void draw_map(const MBState *s) {
    int ax = map_ax(), ay = map_ay(), aw = map_aw(), ah = map_ah();
    float td = s->scale * (float)TILE_SIZE;

    BeginScissorMode(ax, ay, aw, ah);

    int startX = (int)(s->camX / td);
    int startY = (int)(s->camY / td);
    int endX   = startX + (int)(aw / td) + 2;
    int endY   = startY + (int)(ah / td) + 2;
    if (endX > s->map->width)  endX = s->map->width;
    if (endY > s->map->height) endY = s->map->height;

    // Tiles
    for (int ty = startY; ty < endY; ty++) {
        for (int tx = startX; tx < endX; tx++) {
            const MapCell *cell = worldmap_cell(s->map, tx, ty);
            if (!cell) continue;
            tileatlas_draw_cell(&s->atlas, cell, tile_px(s, tx), tile_py(s, ty), s->scale);
        }
    }

    // Grid lines (always on; alpha scales with zoom)
    {
        int sz = (int)td;
        if (sz >= 3) {
            uint8_t alpha = (sz >= 12) ? 55 : (sz >= 6 ? 36 : 20);
            Color gc = {78, 80, 96, alpha};
            for (int ty = startY; ty <= endY; ty++)
                DrawLine(ax, tile_py(s, ty), ax + aw, tile_py(s, ty), gc);
            for (int tx = startX; tx <= endX; tx++)
                DrawLine(tile_px(s, tx), ay, tile_px(s, tx), ay + ah, gc);
        }
    }

    // Rect preview
    if (s->tool == TOOL_RECT && s->rectActive) {
        int rx0 = s->rectX0 < s->rectX1 ? s->rectX0 : s->rectX1;
        int ry0 = s->rectY0 < s->rectY1 ? s->rectY0 : s->rectY1;
        int rx1 = s->rectX0 > s->rectX1 ? s->rectX0 : s->rectX1;
        int ry1 = s->rectY0 > s->rectY1 ? s->rectY0 : s->rectY1;
        int px0 = tile_px(s, rx0), py0 = tile_py(s, ry0);
        int px1 = tile_px(s, rx1 + 1), py1 = tile_py(s, ry1 + 1);
        DrawRectangle(px0, py0, px1 - px0, py1 - py0, C_RECT_FILL);
        DrawRectangleLines(px0, py0, px1 - px0, py1 - py0, C_RECT_BDR);
        char rbuf[32];
        snprintf(rbuf, sizeof(rbuf), "%dx%d", rx1 - rx0 + 1, ry1 - ry0 + 1);
        DrawText(rbuf, px0 + 4, py0 + 4, 11, C_RECT_BDR);
    }

    // Cursor highlight
    Vector2 m = GetMousePosition();
    if (m.x > ax && m.x < ax + aw && m.y > ay && m.y < ay + ah) {
        int tx, ty;
        if (screen_to_tile(s, m.x, m.y, &tx, &ty)) {
            int half = (s->tool == TOOL_PENCIL) ? s->brushSize / 2 : 0;
            int hpx = tile_px(s, tx - half);
            int hpy = tile_py(s, ty - half);
            int hsz = (int)(td * (s->tool == TOOL_PENCIL ? s->brushSize : 1));

            if (s->tool == TOOL_EYEDROP)
                DrawCircleLines(tile_px(s, tx) + (int)(td * 0.5f),
                                tile_py(s, ty) + (int)(td * 0.5f),
                                (int)(td * 0.5f), C_HOVER);
            else
                DrawRectangleLines(hpx, hpy, hsz, hsz, C_HOVER);

            // Tile info overlay
            const MapCell *hc = worldmap_cell(s->map, tx, ty);
            if (hc) {
                char tbuf[80];
                snprintf(tbuf, sizeof(tbuf), "[%d, %d]  %s  v%d",
                         tx, ty, TILE_INFO[hc->type].label, hc->variant);
                int tw = MeasureText(tbuf, 11);
                DrawRectangle(ax + 2, ay + 2, tw + 10, 17, (Color){0, 0, 0, 160});
                DrawText(tbuf, ax + 7, ay + 4, 11, C_TEXT);
            }
        }
    }

    EndScissorMode();
}

// ---------------------------------------------------------------------------
// Draw: status bar
// ---------------------------------------------------------------------------
static void draw_status(const MBState *s) {
    int sy = SCREEN_H - STATUS_H;
    DrawRectangle(0, sy, SCREEN_W, STATUS_H, C_STATUS);
    DrawLine(0, sy, SCREEN_W, sy, C_BORDER);

    char buf[320];
    snprintf(buf, sizeof(buf), "  %s  |  %d x %d tiles  |  zoom %.1fx  |  %s",
             s->filePath, s->map->width, s->map->height,
             s->scale, s->dirty ? "UNSAVED *" : "saved");
    DrawText(buf, 4, sy + 7, 11, C_TEXT_DIM);

    const char *hints =
        "Ctrl+S/O/N/Z  |  P B I R tools  |  [ ] brush  |  +/- variant  |  Home centre";
    int hw = MeasureText(hints, 10);
    DrawText(hints, SCREEN_W - hw - 8, sy + 8, 10, (Color){76, 90, 114, 255});

    if (s->statusTimer > 0.0f) {
        int tw = MeasureText(s->statusMsg, 12);
        int cx = SCREEN_W / 2 - tw / 2;
        DrawRectangle(cx - 10, sy + 2, tw + 20, STATUS_H - 4, (Color){36, 46, 70, 255});
        DrawRectangleLines(cx - 10, sy + 2, tw + 20, STATUS_H - 4, C_BORDER);
        DrawText(s->statusMsg, cx, sy + 7, 12, (Color){255, 216, 68, 255});
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
static void handle_input(MBState *s) {
    float dt = GetFrameTime();
    if (s->statusTimer > 0.0f) s->statusTimer -= dt;

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    Vector2 mouse = GetMousePosition();
    int ax = map_ax(), ay = map_ay(), aw = map_aw(), ah = map_ah();
    bool inMap = (mouse.x > ax && mouse.x < ax + aw &&
                  mouse.y > ay && mouse.y < ay + ah);

    // Tool hotkeys
    if (IsKeyPressed(KEY_P)) s->tool = TOOL_PENCIL;
    if (IsKeyPressed(KEY_B) && !ctrl) s->tool = TOOL_BUCKET;
    if (IsKeyPressed(KEY_I)) s->tool = TOOL_EYEDROP;
    if (IsKeyPressed(KEY_R) && !ctrl) s->tool = TOOL_RECT;

    // Toolbar clicks
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.y < TOOLBAR_H) {
        for (int t = 0; t < TOOL_COUNT; t++) {
            int bx = TB_TOOL_X0 + t * TB_TOOL_W;
            if (mouse.x >= bx && mouse.x < bx + TB_BTN_INNER &&
                mouse.y >= 6 && mouse.y < 6 + TB_BTN_H)
                s->tool = (EditTool)t;
        }
        int brushOpts[3] = {1, 3, 5};
        for (int b = 0; b < 3; b++) {
            int bx = TB_BRUSH_X0 + b * TB_BRUSH_W;
            if (mouse.x >= bx && mouse.x < bx + TB_BRUSH_BTN &&
                mouse.y >= 8 && mouse.y < 8 + TB_BTN_H - 2)
                s->brushSize = brushOpts[b];
        }
    }

    // Brush size
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        if (s->brushSize == 5) s->brushSize = 3;
        else if (s->brushSize == 3) s->brushSize = 1;
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        if (s->brushSize == 1) s->brushSize = 3;
        else if (s->brushSize == 3) s->brushSize = 5;
    }

    // Variant
    {
        int vc = TILE_VARIANT_COUNT[s->selType];
        if (vc > 0 && !ctrl) {
            if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))
                s->selVariant = (s->selVariant + 1) % vc;
            if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))
                s->selVariant = (s->selVariant + vc - 1) % vc;
        }
    }

    // Camera scroll
    float spd = 5.0f * s->scale;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) s->camX += spd;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) s->camX -= spd;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) s->camY += spd;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) s->camY -= spd;
    if (IsKeyPressed(KEY_HOME)) { s->camX = 0.0f; s->camY = 0.0f; }

    float td = s->scale * (float)TILE_SIZE;
    float maxCX = (float)s->map->width  * td - (float)aw;
    float maxCY = (float)s->map->height * td - (float)ah;
    if (s->camX < 0) s->camX = 0;
    if (s->camY < 0) s->camY = 0;
    if (maxCX > 0 && s->camX > maxCX) s->camX = maxCX;
    if (maxCY > 0 && s->camY > maxCY) s->camY = maxCY;

    // Zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && inMap) {
        float old = s->scale;
        s->scale *= (wheel > 0) ? 1.15f : (1.0f / 1.15f);
        if (s->scale < MIN_SCALE) s->scale = MIN_SCALE;
        if (s->scale > MAX_SCALE) s->scale = MAX_SCALE;
        float r = s->scale / old;
        s->camX = r * (s->camX + (mouse.x - ax)) - (mouse.x - ax);
        s->camY = r * (s->camY + (mouse.y - ay)) - (mouse.y - ay);
        if (s->camX < 0) s->camX = 0;
        if (s->camY < 0) s->camY = 0;
    }

    // File ops
    if (ctrl && IsKeyPressed(KEY_S)) {
        if (worldmap_save(s->map, s->filePath)) {
            s->dirty = false;
            char msg[320]; snprintf(msg, sizeof(msg), "Saved: %s", s->filePath);
            show_status(s, msg);
        } else show_status(s, "ERROR: save failed");
    }
    if (ctrl && IsKeyPressed(KEY_O)) {
        WorldMap *m2 = worldmap_load(s->filePath);
        if (m2) {
            worldmap_free(s->map); s->map = m2; s->dirty = false;
            char msg[320]; snprintf(msg, sizeof(msg), "Loaded: %s", s->filePath);
            show_status(s, msg);
        } else show_status(s, "ERROR: load failed");
    }
    if (ctrl && IsKeyPressed(KEY_N)) {
        save_undo(s);
        worldmap_free(s->map);
        s->map = worldmap_create(DEFAULT_MAP_W, DEFAULT_MAP_H);
        s->dirty = true;
        show_status(s, "New map (80 x 60)");
    }
    if (ctrl && IsKeyPressed(KEY_Z)) {
        if (s->undo) {
            WorldMap *tmp = s->map; s->map = s->undo; s->undo = tmp;
            s->dirty = true;
            show_status(s, "Undo");
        } else show_status(s, "Nothing to undo");
    }

    if (!inMap) {
        s->painting = false;
        if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) s->rectActive = false;
        return;
    }

    // Map painting
    bool leftDown  = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool rightDown = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    bool leftPres  = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool rightPres = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
    bool leftRel   = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    int tx, ty;
    bool onTile = screen_to_tile(s, mouse.x, mouse.y, &tx, &ty);

    switch (s->tool) {
        case TOOL_PENCIL:
            if (leftDown || rightDown) {
                if (!s->painting) { save_undo(s); s->painting = true; }
                if (onTile) {
                    if (leftDown) {
                        paint_brush(s, tx, ty);
                    } else {
                        int half = s->brushSize / 2;
                        for (int ddy = -half; ddy <= half; ddy++) {
                            for (int ddx = -half; ddx <= half; ddx++) {
                                MapCell *c = worldmap_cell(s->map, tx+ddx, ty+ddy);
                                if (c) { c->type = TILE_GRASS; c->variant = 2; }
                            }
                        }
                        s->dirty = true;
                    }
                }
            } else s->painting = false;
            break;

        case TOOL_BUCKET:
            if (leftPres && onTile) {
                save_undo(s); flood_fill(s, tx, ty); s->dirty = true;
            } else if (rightPres && onTile) {
                TileType oldType = s->selType; int oldVar = s->selVariant;
                s->selType = TILE_GRASS; s->selVariant = 2;
                save_undo(s); flood_fill(s, tx, ty); s->dirty = true;
                s->selType = oldType; s->selVariant = oldVar;
            }
            break;

        case TOOL_EYEDROP:
            if (leftPres && onTile) {
                const MapCell *c = worldmap_cell(s->map, tx, ty);
                if (c) {
                    s->selType    = (TileType)c->type;
                    s->selVariant = (int)c->variant;
                    s->tool = TOOL_PENCIL;
                    show_status(s, "Picked — switched to Pencil");
                }
            }
            break;

        case TOOL_RECT:
            if (leftPres && onTile) {
                s->rectActive = true;
                s->rectX0 = s->rectX1 = tx;
                s->rectY0 = s->rectY1 = ty;
            } else if (leftDown && s->rectActive && onTile) {
                s->rectX1 = tx; s->rectY1 = ty;
            } else if (leftRel && s->rectActive) {
                save_undo(s);
                int rx0 = s->rectX0 < s->rectX1 ? s->rectX0 : s->rectX1;
                int ry0 = s->rectY0 < s->rectY1 ? s->rectY0 : s->rectY1;
                int rx1 = s->rectX0 > s->rectX1 ? s->rectX0 : s->rectX1;
                int ry1 = s->rectY0 > s->rectY1 ? s->rectY0 : s->rectY1;
                for (int y = ry0; y <= ry1; y++) {
                    for (int x = rx0; x <= rx1; x++) {
                        MapCell *c = worldmap_cell(s->map, x, y);
                        if (c) { c->type=(uint8_t)s->selType; c->variant=(uint8_t)s->selVariant; }
                    }
                }
                s->dirty = true;
                s->rectActive = false;
            }
            if (rightDown) s->rectActive = false;
            break;

        default: break;
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Economy Sandbox - Map Builder");
    SetTargetFPS(60);

    MBState s = {0};
    s.map        = worldmap_create(DEFAULT_MAP_W, DEFAULT_MAP_H);
    s.selType    = TILE_GRASS;
    s.selVariant = 2;
    s.scale      = 3.0f;
    s.brushSize  = 1;
    s.tool       = TOOL_PENCIL;
    strncpy(s.filePath, DEFAULT_FILE, sizeof(s.filePath) - 1);

    tileatlas_load(&s.atlas);

    WorldMap *loaded = worldmap_load(DEFAULT_FILE);
    if (loaded) {
        worldmap_free(s.map);
        s.map = loaded;
        show_status(&s, "Loaded: " DEFAULT_FILE);
    } else {
        show_status(&s, "New map  --  " DEFAULT_FILE " not found");
    }

    while (!WindowShouldClose()) {
        handle_input(&s);

        BeginDrawing();
        ClearBackground(C_BG);
        draw_map(&s);
        draw_palette(&s);
        draw_toolbar(&s);
        draw_status(&s);
        EndDrawing();
    }

    tileatlas_unload(&s.atlas);
    worldmap_free(s.map);
    if (s.undo) worldmap_free(s.undo);
    CloseWindow();
    return 0;
}
