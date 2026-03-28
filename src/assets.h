#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

#define SPRITE_TYPE_COUNT  10
#define SPRITE_FRAME_SIZE  32   // each cell in the sheet is 32x32 px
#define SPRITE_WALK_FRAMES  6   // 6 columns per animation row
#define SPRITE_WALK_ROW     1   // row 1 = side-walk (west); flip for east

#define HOUSE_COUNT 6

typedef struct {
    Texture2D background;
    Texture2D sprites[SPRITE_TYPE_COUNT];
    Texture2D houses[HOUSE_COUNT];
} Assets;

void assets_load(Assets *a);
void assets_unload(Assets *a);

#endif
