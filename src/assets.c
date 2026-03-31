#include "assets.h"

static const char *WORKER_PATHS[WORKER_TYPE_COUNT] = {
    "src/assets/MiniWorldSprites/Characters/Workers/CyanWorker/FarmerCyan.png",
    "src/assets/MiniWorldSprites/Characters/Workers/LimeWorker/FarmerLime.png",
    "src/assets/MiniWorldSprites/Characters/Workers/PurpleWorker/FarmerPurple.png",
    "src/assets/MiniWorldSprites/Characters/Workers/RedWorker/FarmerRed.png",
};

void assets_load(Assets *a) {
    for (int i = 0; i < WORKER_TYPE_COUNT; i++) {
        a->workers[i] = LoadTexture(WORKER_PATHS[i]);
        SetTextureFilter(a->workers[i], TEXTURE_FILTER_POINT);
    }
}

void assets_unload(Assets *a) {
    for (int i = 0; i < WORKER_TYPE_COUNT; i++) {
        UnloadTexture(a->workers[i]);
    }
}
