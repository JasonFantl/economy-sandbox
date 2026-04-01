#include "agent_render.h"
#include "render.h"
#include "raylib.h"

// Draw one agent using the MiniWorldSprites worker sheet.
// Worker sheet layout (80×192 = 5 cols × 12 rows of 16×16):
//   rows 0-2  = south walk,  rows 3-5 = west walk
//   rows 6-8  = east walk,   rows 9-11= north walk
//   cols 0-2 = animation frames we cycle
void draw_agent(const Agent *a, const Assets *assets) {
    int dirRow = (int)a->sprite.facing * WORKER_DIR_ROWS; // 0,3,6,9
    int col    = a->sprite.animFrame;                     // 0,1,2
    float srcX = (float)(col    * WORKER_FRAME_W);
    float srcY = (float)(dirRow * WORKER_FRAME_H);

    Rectangle src = { srcX, srcY, (float)WORKER_FRAME_W, (float)WORKER_FRAME_H };
    float disp = (float)AGENT_DISP;
    Rectangle dst = { a->body.x - disp * 0.5f,
                      a->body.y - disp * 0.5f,
                      disp, disp };

    Color tint = (a->sprite.tradeFlash > 0.0f) ? YELLOW : WHITE;
    DrawTexturePro(assets->workers[a->sprite.spriteType % WORKER_TYPE_COUNT],
                   src, dst, (Vector2){0,0}, 0.0f, tint);
}
