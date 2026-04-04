#include "render/render.h"
#include "render/panels.h"
#include "econ/agent.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// World viewport Y offset — 0 in free play, NAV_H in walkthrough mode
// ---------------------------------------------------------------------------
int g_world_view_y = 0;

// ---------------------------------------------------------------------------
// Bounds editing state — per (PlotType x MarketId); survives plot cycling
// ---------------------------------------------------------------------------
// g_bounds is defined in panels.c; we use it via the extern in panels.h

// At most one bounds field is being edited at a time
static int      g_bnd_side = -1;   // panel index 0-3, or -1=none
static PlotType g_bnd_pt   = 0;
static int      g_bnd_mid  = 0;
static int      g_bnd_axis = 0;    // 0=yMax, 1=xMax
static char     g_bnd_buf[16];
static int      g_bnd_len  = 0;

// ---------------------------------------------------------------------------
// World — 2D top-down tile map + agents
// ---------------------------------------------------------------------------

// Depth-sort agents by Y (painter's algorithm) without allocating
static int s_sort_count;
static const Agent *s_sort_agents_ptr;
static int cmp_agent_y(const void *a, const void *b) {
    float ya = s_sort_agents_ptr[*(const int*)a].body.y;
    float yb = s_sort_agents_ptr[*(const int*)b].body.y;
    return (ya > yb) - (ya < yb);
}

void render_world(const WorldMap *map, const TileAtlas *tiles,
                  const Agent *agents, int count,
                  bool paused, int simSteps,
                  const Assets *assets,
                  float camX, float camY, float camZoom) {
    // Camera: world point (camX, camY) is centred in the viewport.
    Camera2D cam = {0};
    cam.offset   = (Vector2){ (float)SCREEN_W * 0.5f, (float)g_world_view_y + (float)WORLD_VIEW_H * 0.5f };
    cam.target   = (Vector2){ camX, camY };
    cam.zoom     = camZoom;

    // Compute visible tile range for frustum culling
    float tileW = (float)TILE_SIZE * WORLD_TILE_SCALE;
    float worldLeft   = camX - ((float)SCREEN_W   * 0.5f) / camZoom;
    float worldTop    = camY - ((float)WORLD_VIEW_H * 0.5f) / camZoom;
    float worldRight  = worldLeft + (float)SCREEN_W   / camZoom;
    float worldBottom = worldTop  + (float)WORLD_VIEW_H / camZoom;
    int txMin = (int)(worldLeft  / tileW) - 1;
    int tyMin = (int)(worldTop   / tileW) - 1;
    int txMax = (int)(worldRight  / tileW) + 2;
    int tyMax = (int)(worldBottom / tileW) + 2;

    BeginScissorMode(0, g_world_view_y, SCREEN_W, WORLD_VIEW_H);
    BeginMode2D(cam);

    if (map) {
        int x0 = txMin < 0 ? 0 : txMin,  x1 = txMax > map->width  ? map->width  : txMax;
        int y0 = tyMin < 0 ? 0 : tyMin,  y1 = tyMax > map->height ? map->height : tyMax;
        // Pass 1 — ground layer
        for (int ty = y0; ty < y1; ty++) {
            for (int tx = x0; tx < x1; tx++) {
                const MapCell *cell = worldmap_cell(map, tx, ty);
                if (!cell) continue;
                int px = (int)((float)tx * tileW);
                int py = (int)((float)ty * tileW);
                tileatlas_draw_cell(tiles, cell, px, py, WORLD_TILE_SCALE);
            }
        }
        // Pass 2 — object layer (PNG transparency lets ground show through)
        for (int ty = y0; ty < y1; ty++) {
            for (int tx = x0; tx < x1; tx++) {
                const MapCell *cell = worldmap_obj_cell(map, tx, ty);
                if (!cell || cell->type == TILE_NONE) continue;
                int px = (int)((float)tx * tileW);
                int py = (int)((float)ty * tileW);
                tileatlas_draw_cell(tiles, cell, px, py, WORLD_TILE_SCALE);
            }
        }
    } else {
        // Fallback: solid grass background
        DrawRectangle((int)worldLeft, (int)worldTop,
                      (int)((float)SCREEN_W / camZoom),
                      (int)((float)WORLD_VIEW_H / camZoom),
                      (Color){60, 120, 50, 255});
    }

    // --- Agents (depth-sorted by Y) ---
    static int indices[MAX_AGENTS];
    for (int i = 0; i < count; i++) indices[i] = i;
    s_sort_agents_ptr = agents;
    s_sort_count = count;
    (void)s_sort_count;
    qsort(indices, (size_t)count, sizeof(int), cmp_agent_y);
    for (int i = 0; i < count; i++) {
        draw_agent(&agents[indices[i]], assets);
    }

    EndMode2D();
    EndScissorMode();

    // Divider between world and plots
    DrawRectangle(0, g_world_view_y + WORLD_VIEW_H, SCREEN_W, 2, DARKGRAY);

    // Legend
    DrawRectangle(0, g_world_view_y, 440, 26, (Color){0,0,0,120});
    DrawCircle( 10, g_world_view_y+13, 4,(Color){150,150,150,255}); DrawText("Leisure",  18, g_world_view_y+6, 13, WHITE);
    DrawCircle( 90, g_world_view_y+13, 4,(Color){160,100, 40,255}); DrawText("Chopping", 98, g_world_view_y+6, 13, WHITE);
    DrawCircle(190, g_world_view_y+13, 4,(Color){220,140, 60,255}); DrawText("Building",198, g_world_view_y+6, 13, WHITE);
    DrawCircle(285, g_world_view_y+13, 4, YELLOW);                  DrawText("Trading", 293, g_world_view_y+6, 13, WHITE);

    // Speed indicator
    char speedBuf[32]; Color speedCol;
    if      (paused)       { snprintf(speedBuf,sizeof(speedBuf),"PAUSED");              speedCol=RED;    }
    else if (simSteps > 1) { snprintf(speedBuf,sizeof(speedBuf),"Speed: %dx",simSteps); speedCol=ORANGE; }
    else                   { snprintf(speedBuf,sizeof(speedBuf),"Speed: 1x");           speedCol=WHITE;  }
    DrawRectangle(SCREEN_W-200, g_world_view_y, 200, 40, (Color){0,0,0,100});
    DrawText(speedBuf,    SCREEN_W-108, g_world_view_y+4,  16, speedCol);
    DrawText("[SPACE]/[F]",SCREEN_W-186, g_world_view_y+24, 11, (Color){200,200,200,255});
}

// ---------------------------------------------------------------------------
// Panel strips
// ---------------------------------------------------------------------------

static const char *plot_title(PlotType t) {
    switch (t) {
        case PLOT_WEALTH:                 return "Money vs Goods";
        case PLOT_VALUATION_DISTRIBUTION: return "Valuation Distribution";
        case PLOT_PRICE_HISTORY:          return "Price History";
        case PLOT_GOODS_HISTORY:          return "Goods History";
        case PLOT_SUPPLY_DEMAND:          return "Supply & Demand";
        default:                 return "";
    }
}
static const char *market_title(int mid) {
    return (mid == MARKET_WOOD) ? "Wood" : "Chair";
}

static void draw_plot_strip(int px, int py_strip, int pw, PlotType t) {
    Vector2 m=GetMousePosition();
    bool hover=m.x>=px&&m.x<=px+pw&&m.y>=py_strip&&m.y<=py_strip+STRIP_H;
    Color bg  =hover?(Color){50,65,95,230} :(Color){28,38,58,210};
    Color bdr =hover?(Color){100,130,180,255}:(Color){55,72,100,200};
    DrawRectangle(px,py_strip,pw,STRIP_H,bg);
    DrawRectangleLines(px,py_strip,pw,STRIP_H,bdr);
    char lbl[64]; snprintf(lbl,sizeof(lbl),"< %s >",plot_title(t));
    int tw=MeasureText(lbl,12);
    DrawText(lbl,px+(pw-tw)/2,py_strip+2,12,hover?WHITE:(Color){180,190,210,255});
}

static void draw_market_strip(int px, int py_strip, int pw, int marketId) {
    Vector2 m=GetMousePosition();
    bool hover=m.x>=px&&m.x<=px+pw&&m.y>=py_strip&&m.y<=py_strip+STRIP_H;
    Color mktColor=(marketId==MARKET_WOOD)?(Color){160,100,40,255}:(Color){200,140,60,255};
    Color bg  =hover?(Color){50,40,25,230} :(Color){32,24,12,210};
    Color bdr =hover?(Color){180,120,60,255}:(Color){90,65,30,200};
    DrawRectangle(px,py_strip,pw,STRIP_H,bg);
    DrawRectangleLines(px,py_strip,pw,STRIP_H,bdr);
    char lbl[32]; snprintf(lbl,sizeof(lbl),"< %s >",market_title(marketId));
    int tw=MeasureText(lbl,12);
    DrawText(lbl,px+(pw-tw)/2,py_strip+2,12,hover?WHITE:mktColor);
}

static void draw_bounds_strip(int px, int py_strip, int pw,
                               int panelIdx, PlotType pt, int mid) {
    PlotBounds *b=&g_bounds[pt][mid];
    Vector2 mouse=GetMousePosition();
    bool hover=mouse.x>=px&&mouse.x<=px+pw&&mouse.y>=py_strip&&mouse.y<=py_strip+STRIP_H;
    bool editing=(g_bnd_side==panelIdx&&g_bnd_pt==pt&&g_bnd_mid==mid);

    Color bg  =editing?(Color){20,50,20,230} :(hover?(Color){25,40,25,230}:(Color){15,28,15,210});
    Color bdr =editing?(Color){80,160,80,255}:(Color){40,80,40,200};
    DrawRectangle(px,py_strip,pw,STRIP_H,bg);
    DrawRectangleLines(px,py_strip,pw,STRIP_H,bdr);

    bool cur=(int)(GetTime()*2)%2==0;
    if (pt==PLOT_WEALTH) {
        int half=pw/2;
        DrawLine(px+half,py_strip+2,px+half,py_strip+STRIP_H-2,(Color){50,80,50,200});
        char xs[20],ys[20];
        if (editing&&g_bnd_axis==1) snprintf(xs,sizeof(xs),"%s%s",g_bnd_buf,cur?"|":" ");
        else snprintf(xs,sizeof(xs),b->xMax>0?"%.0f":"auto",b->xMax);
        if (editing&&g_bnd_axis==0) snprintf(ys,sizeof(ys),"%s%s",g_bnd_buf,cur?"|":" ");
        else snprintf(ys,sizeof(ys),b->yMax>0?"%.0f":"auto",b->yMax);
        Color xCol=(editing&&g_bnd_axis==1)?WHITE:(b->xMax>0?(Color){200,220,100,255}:(Color){100,120,80,200});
        Color yCol=(editing&&g_bnd_axis==0)?WHITE:(b->yMax>0?(Color){200,220,100,255}:(Color){100,120,80,200});
        char lbl[24]; snprintf(lbl,sizeof(lbl),"X: %s",xs);
        int lw=MeasureText(lbl,11); DrawText(lbl,px+(half-lw)/2,py_strip+3,11,xCol);
        snprintf(lbl,sizeof(lbl),"Y: %s",ys);
        int rw=MeasureText(lbl,11); DrawText(lbl,px+half+(half-rw)/2,py_strip+3,11,yCol);
    } else {
        char ys[20];
        if (editing) snprintf(ys,sizeof(ys),"%s%s",g_bnd_buf,cur?"|":" ");
        else snprintf(ys,sizeof(ys),b->yMax>0?"%.0f":"auto",b->yMax);
        Color yCol=editing?WHITE:(b->yMax>0?(Color){200,220,100,255}:(Color){100,120,80,200});
        char lbl[32]; snprintf(lbl,sizeof(lbl),"Y max: %s",ys);
        int tw=MeasureText(lbl,11);
        DrawText(lbl,px+(pw-tw)/2,py_strip+3,11,yCol);
    }
    if (!editing) {
        const char *hint="(0=auto)";
        DrawText(hint,px+pw-MeasureText(hint,9)-3,py_strip+4,9,(Color){60,80,60,200});
    }
}

// ---------------------------------------------------------------------------
// Top-level draw dispatch
// ---------------------------------------------------------------------------

static void draw_panel(const PanelState *ps,
                       const AgentValueHistory avh[MARKET_COUNT],
                       const AgentValueHistory pvh[MARKET_COUNT],
                       const AgentValueHistory gvh[MARKET_COUNT],
                       const Agent *agents, int agentCount,
                       int px, int py, int pw, int ph, float equilibrium) {
    int mid=ps->marketId;
    switch (ps->plotType) {
        case PLOT_WEALTH:
            panel_wealth(agents, agentCount, mid, px, py, pw, ph); break;
        case PLOT_VALUATION_DISTRIBUTION:
            panel_valuation_dist(agents, agentCount, mid, px, py, pw, ph, equilibrium); break;
        case PLOT_PRICE_HISTORY:
            panel_price_history(&avh[mid], &pvh[mid], agents, agentCount, mid, px, py, pw, ph, equilibrium, true); break;
        case PLOT_GOODS_HISTORY:
            panel_goods_history(&gvh[mid], mid, px, py, pw, ph); break;
        case PLOT_SUPPLY_DEMAND:
            panel_supply_demand(agents, agentCount, mid, px, py, pw, ph); break;
        default: break;
    }
}

// Compute panel geometry for a given panel index (0=TL,1=TR,2=BL,3=BR)
static void panel_geom(int panelIdx,
                       int *out_px, int *out_strip_y, int *out_py, int *out_ph) {
    int half     = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int px_col[2] = { PLOT_MARGIN_L, PLOT_MARGIN_L + half + PANEL_GAP };
    int plots_y  = g_world_view_y + WORLD_VIEW_H + 2;
    int plots_h  = SCREEN_H - plots_y;
    int row_h    = (plots_h - PANEL_ROW_GAP) / 2;
    int row_y[2] = { plots_y, plots_y + row_h + PANEL_ROW_GAP };

    int col = panelIdx % 2;
    int row = panelIdx / 2;

    *out_px      = px_col[col];
    *out_strip_y = row_y[row];
    *out_py      = row_y[row] + 3*STRIP_H + PLOT_MARGIN_T;
    *out_ph      = row_h - 3*STRIP_H - PLOT_MARGIN_T - PLOT_MARGIN_B;
    (void)half;  // stored in px_col
}

void render_panels_freeplay(const AgentValueHistory avh[MARKET_COUNT],
                             const AgentValueHistory pvh[MARKET_COUNT],
                             const AgentValueHistory gvh[MARKET_COUNT],
                             const Agent *agents, int agentCount,
                             PanelState panels[NUM_PANELS]) {
    int half     = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int plots_y  = g_world_view_y + WORLD_VIEW_H + 2;
    int plots_h  = SCREEN_H - plots_y;
    int row_h    = (plots_h - PANEL_ROW_GAP) / 2;
    int px_right = PLOT_MARGIN_L + half + PANEL_GAP;
    int row1_y   = plots_y + row_h + PANEL_ROW_GAP;

    DrawRectangle(0,plots_y,SCREEN_W,SCREEN_H-plots_y,(Color){20,20,30,255});

    // Vertical and horizontal dividers
    DrawLine(px_right-PANEL_GAP/2, plots_y+2, px_right-PANEL_GAP/2, SCREEN_H-2, (Color){60,60,70,255});
    DrawLine(0, row1_y-PANEL_ROW_GAP/2, SCREEN_W, row1_y-PANEL_ROW_GAP/2, (Color){60,60,70,255});

    for (int pi=0; pi<NUM_PANELS; pi++) {
        int ppx, strip_y, py, ph;
        panel_geom(pi, &ppx, &strip_y, &py, &ph);

        float eq=0.0f;
        for (int i=0;i<agentCount;i++)
            eq+=agents[i].econ.markets[panels[pi].marketId].maxUtility;
        eq/=(float)agentCount;

        draw_plot_strip  (ppx, strip_y,          half, panels[pi].plotType);
        draw_market_strip(ppx, strip_y+STRIP_H,  half, panels[pi].marketId);
        draw_bounds_strip(ppx, strip_y+2*STRIP_H,half, pi, panels[pi].plotType, panels[pi].marketId);

        draw_panel(&panels[pi], avh, pvh, gvh, agents, agentCount, ppx, py, half, ph, eq);
    }
}

// Keep old name as alias for backward compatibility
void render_plot(const AgentValueHistory avh[MARKET_COUNT],
                 const AgentValueHistory pvh[MARKET_COUNT],
                 const AgentValueHistory gvh[MARKET_COUNT],
                 const Agent *agents, int agentCount,
                 PanelState panels[NUM_PANELS]) {
    render_panels_freeplay(avh, pvh, gvh, agents, agentCount, panels);
}

// ---------------------------------------------------------------------------
// Input handlers
// ---------------------------------------------------------------------------

static void cancel_bounds_edit(void) { g_bnd_side = -1; }

bool panel_handle_click(PanelState panels[NUM_PANELS]) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;
    Vector2 m=GetMousePosition();

    int half=(SCREEN_W-PLOT_MARGIN_L-PLOT_MARGIN_R-PANEL_GAP)/2;

    for (int pi=0; pi<NUM_PANELS; pi++) {
        int ppx, strip_y, py, ph;
        panel_geom(pi, &ppx, &strip_y, &py, &ph);
        (void)py; (void)ph;

        if (m.y < strip_y || m.y >= strip_y+3*STRIP_H) continue;
        if (m.x < ppx     || m.x >  ppx+half)          continue;

        if (m.y < strip_y+STRIP_H) {
            cancel_bounds_edit();
            panels[pi].plotType=(PlotType)((panels[pi].plotType+1)%PLOT_COUNT);
            return true;
        }
        if (m.y < strip_y+2*STRIP_H) {
            cancel_bounds_edit();
            panels[pi].marketId=(panels[pi].marketId+1)%MARKET_COUNT;
            return true;
        }

        // Bounds strip
        PlotType pt=panels[pi].plotType;
        int mid=panels[pi].marketId;
        PlotBounds *b=&g_bounds[pt][mid];
        int axis=(pt==PLOT_WEALTH)?(m.x<ppx+half/2?1:0):0;

        if (g_bnd_side==pi && g_bnd_pt==pt && g_bnd_mid==mid && g_bnd_axis==axis) {
            float val=(float)atof(g_bnd_buf);
            if (val<0.0f) val=0.0f;
            if (axis==0) b->yMax=val; else b->xMax=val;
            g_bnd_side=-1;
        } else {
            g_bnd_side=pi; g_bnd_pt=pt; g_bnd_mid=mid; g_bnd_axis=axis;
            float curVal=(axis==0)?b->yMax:b->xMax;
            if (curVal>0.0f) snprintf(g_bnd_buf,sizeof(g_bnd_buf),"%.0f",curVal);
            else g_bnd_buf[0]='\0';
            g_bnd_len=(int)strlen(g_bnd_buf);
        }
        return true;
    }
    return false;
}

bool panel_handle_bounds_keyboard(void) {
    if (g_bnd_side<0) return false;
    int ch;
    while ((ch=GetCharPressed())!=0) {
        bool ok=(ch>='0'&&ch<='9')||ch=='.';
        if (ok&&g_bnd_len<14) { g_bnd_buf[g_bnd_len++]=(char)ch; g_bnd_buf[g_bnd_len]='\0'; }
    }
    if (IsKeyPressed(KEY_BACKSPACE)&&g_bnd_len>0) g_bnd_buf[--g_bnd_len]='\0';
    if (IsKeyPressed(KEY_ENTER)) {
        float val=(float)atof(g_bnd_buf);
        if (val<0.0f) val=0.0f;
        PlotBounds *b=&g_bounds[g_bnd_pt][g_bnd_mid];
        if (g_bnd_axis==0) b->yMax=val; else b->xMax=val;
        g_bnd_side=-1;
    }
    if (IsKeyPressed(KEY_ESCAPE)) g_bnd_side=-1;
    return true;
}
