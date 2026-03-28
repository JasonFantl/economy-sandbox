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

void assets_load(Assets *a) {
    a->background = LoadTexture(BG_PATH);
    SetTextureFilter(a->background, TEXTURE_FILTER_POINT);

    a->village = LoadTexture(
        "src/assets/Village/Village/Set1_Ourdoor_Decoration.png");
    SetTextureFilter(a->village, TEXTURE_FILTER_POINT);

    for (int i = 0; i < SPRITE_TYPE_COUNT; i++) {
        a->sprites[i] = LoadTexture(SPRITE_PATHS[i]);
        SetTextureFilter(a->sprites[i], TEXTURE_FILTER_POINT);
    }
}

void assets_unload(Assets *a) {
    UnloadTexture(a->background);
    UnloadTexture(a->village);
    for (int i = 0; i < SPRITE_TYPE_COUNT; i++) {
        UnloadTexture(a->sprites[i]);
    }
}
