#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

// ---------------------------------------------------------------------------
// Worker sprite sheet layout (MiniWorldSprites/Characters/Workers)
// Sheet dimensions: 80×192 px = 5 cols × 12 rows of 16×16 frames
// Row groups:  0-2 = walk south, 3-5 = walk west, 6-8 = walk east, 9-11 = walk north
// Columns:     0 = idle pose, 1-4 = walk cycle (we use columns 0-2 for 3-frame anim)
// ---------------------------------------------------------------------------
#define WORKER_FRAME_W     16
#define WORKER_FRAME_H     16
#define WORKER_COLS         5    // columns in the sprite sheet
#define WORKER_WALK_FRAMES  3    // animation frames we cycle through (cols 0-2)
#define WORKER_DIR_ROWS     3    // rows per direction
#define WORKER_TYPE_COUNT   4    // Cyan, Lime, Purple, Red

typedef struct {
    Texture2D workers[WORKER_TYPE_COUNT];
} Assets;

void assets_load(Assets *a);
void assets_unload(Assets *a);

#endif
