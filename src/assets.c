#include "assets.h"

// Paths relative to the project root (where ./game is run from)
static const char *BG_PATH =
    "src/assets/Nature Landscapes Free Pixel Art/nature_2/orig.png";

static const char *SPRITE_PATHS[SPRITE_TYPE_COUNT] = {
    "src/assets/MinifolksVillagers/Outline/MiniVillagerMan.png",
    "src/assets/MinifolksVillagers/Outline/MiniVillagerWoman.png",
    "src/assets/MinifolksVillagers/Outline/MiniPeasant.png",
    "src/assets/MinifolksVillagers/Outline/MiniWorker.png",
    "src/assets/MinifolksVillagers/Outline/MiniNobleMan.png",
    "src/assets/MinifolksVillagers/Outline/MiniNobleWoman.png",
    "src/assets/MinifolksVillagers/Outline/MiniOldMan.png",
    "src/assets/MinifolksVillagers/Outline/MiniOldWoman.png",
    "src/assets/MinifolksVillagers/Outline/MiniPrincess.png",
    "src/assets/MinifolksVillagers/Outline/MiniQueen.png",
};

static const char *HOUSE_PATHS[HOUSE_COUNT] = {
    "src/assets/pixel_village/houses/house1.png",
    "src/assets/pixel_village/houses/house2.png",
    "src/assets/pixel_village/houses/house3.png",
    "src/assets/pixel_village/houses/house4.png",
    "src/assets/pixel_village/houses/house5.png",
    "src/assets/pixel_village/houses/house6.png",
};

void assets_load(Assets *a) {
    a->background = LoadTexture(BG_PATH);
    SetTextureFilter(a->background, TEXTURE_FILTER_POINT);

    for (int i = 0; i < SPRITE_TYPE_COUNT; i++) {
        a->sprites[i] = LoadTexture(SPRITE_PATHS[i]);
        SetTextureFilter(a->sprites[i], TEXTURE_FILTER_POINT);
    }

    for (int i = 0; i < HOUSE_COUNT; i++) {
        a->houses[i] = LoadTexture(HOUSE_PATHS[i]);
        SetTextureFilter(a->houses[i], TEXTURE_FILTER_POINT);
    }
}

void assets_unload(Assets *a) {
    UnloadTexture(a->background);
    for (int i = 0; i < SPRITE_TYPE_COUNT; i++) {
        UnloadTexture(a->sprites[i]);
    }
    for (int i = 0; i < HOUSE_COUNT; i++) {
        UnloadTexture(a->houses[i]);
    }
}
