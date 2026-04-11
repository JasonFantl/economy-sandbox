#include "render/camera.h"
#include "render/render.h"
#include "raylib.h"

#define CAM_ZOOM_MIN 0.25f
#define CAM_ZOOM_MAX 6.0f

float g_camX    = 0.0f;
float g_camY    = 0.0f;
float g_camZoom = 1.0f;

void camera_init(void) {
    g_camX = (float)GetScreenWidth() * 0.5f;
    g_camY = (float)WORLD_VIEW_H     * 0.5f;
}

void camera_update(int worldViewY) {
    Vector2 mouse = GetMousePosition();
    bool inWorld = mouse.y >= (float)worldViewY &&
                   mouse.y <= (float)worldViewY + (float)WORLD_VIEW_H;

    if (inWorld && IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 delta = GetMouseDelta();
        g_camX -= delta.x / g_camZoom;
        g_camY -= delta.y / g_camZoom;
    }

    if (inWorld) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            float cx   = (float)GetScreenWidth() * 0.5f;
            float cy   = (float)worldViewY + (float)WORLD_VIEW_H * 0.5f;
            float preWx = (mouse.x - cx) / g_camZoom + g_camX;
            float preWy = (mouse.y - cy) / g_camZoom + g_camY;
            float factor = (wheel > 0.0f) ? 1.1f : 1.0f / 1.1f;
            g_camZoom *= factor;
            if (g_camZoom < CAM_ZOOM_MIN) g_camZoom = CAM_ZOOM_MIN;
            if (g_camZoom > CAM_ZOOM_MAX) g_camZoom = CAM_ZOOM_MAX;
            g_camX = preWx - (mouse.x - cx) / g_camZoom;
            g_camY = preWy - (mouse.y - cy) / g_camZoom;
        }
    }
}
