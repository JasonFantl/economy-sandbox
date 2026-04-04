#ifndef CAMERA_H
#define CAMERA_H

extern float g_camX;
extern float g_camY;
extern float g_camZoom;

// Set camera to its default starting position
void camera_init(void);

// Handle middle-mouse pan and scroll-wheel zoom; worldViewY is the top pixel of
// the world viewport (0 in free-play, WTHROUGH_NAV_H in walkthrough mode)
void camera_update(int worldViewY);

#endif
