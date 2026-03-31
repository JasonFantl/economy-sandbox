#include "render.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Bounds state — per (PlotType × MarketId); survives plot cycling
// ---------------------------------------------------------------------------

typedef struct { float xMax; float yMax; } PlotBounds;  // 0 = auto
static PlotBounds g_bounds[PLOT_COUNT][MARKET_COUNT];

// At most one bounds field is being edited at a time
static int      g_bnd_side = -1;   // panel index 0-3, or -1=none
static PlotType g_bnd_pt   = 0;
static int      g_bnd_mid  = 0;
static int      g_bnd_axis = 0;    // 0=yMax, 1=xMax
static char     g_bnd_buf[16];
static int      g_bnd_len  = 0;

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

#define SPRITE_DISP  (SPRITE_FRAME_SIZE * SPRITE_SCALE)

typedef struct { int houseIdx; float x; float scale; } HouseDecor;

// Reduced house scale (3.5→2.5) to fit the smaller world area
static const HouseDecor HOUSES[] = {
    {0,   80, 2.5f}, {2,  220, 2.5f}, {4,  360, 2.5f},
    {1,  500, 2.5f}, {5,  640, 2.5f}, {3,  780, 2.5f},
    {0,  920, 2.5f}, {2, 1060, 2.5f}, {4, 1180, 2.5f},
};
static const int HOUSE_DECOR_COUNT = 9;

static void draw_house(const HouseDecor *h, const Assets *assets) {
    Texture2D tex = assets->houses[h->houseIdx];
    float dw = (float)tex.width  * h->scale;
    float dh = (float)tex.height * h->scale;
    DrawTexturePro(tex, (Rectangle){0,0,(float)tex.width,(float)tex.height},
                   (Rectangle){h->x-dw/2.0f,(float)GROUND_Y-dh,dw,dh},
                   (Vector2){0,0}, 0.0f, WHITE);
}

static void draw_agent(const Agent *a, const Assets *assets) {
    float fw = a->sprite.facingRight ? (float)SPRITE_FRAME_SIZE : -(float)SPRITE_FRAME_SIZE;
    float fx = a->sprite.facingRight ? (float)(a->sprite.animFrame*SPRITE_FRAME_SIZE)
                                     : (float)((a->sprite.animFrame+1)*SPRITE_FRAME_SIZE);
    Rectangle src = {fx,(float)(SPRITE_WALK_ROW*SPRITE_FRAME_SIZE),fw,(float)SPRITE_FRAME_SIZE};
    Rectangle dst = {a->body.x-SPRITE_DISP/2.0f,(float)GROUND_Y-SPRITE_DISP,SPRITE_DISP,SPRITE_DISP};
    DrawTexturePro(assets->sprites[a->sprite.spriteType], src, dst,
                   (Vector2){0,0}, 0.0f, (a->sprite.tradeFlash>0.0f)?YELLOW:WHITE);

    Color dot;
    if      (a->sprite.tradeFlash > 0.0f)          dot = YELLOW;
    else if (a->econ.lastAction == ACTION_CHOP)     dot = (Color){160,100, 40,220};
    else if (a->econ.lastAction == ACTION_BUILD)    dot = (Color){220,140, 60,220};
    else                                            dot = (Color){150,150,150,180};
    DrawCircle((int)a->body.x, GROUND_Y+3, 3, dot);
}

void render_world(const Agent *agents, int count, bool paused, int simSteps,
                  const Assets *assets) {
    DrawTexturePro(assets->background,
        (Rectangle){0,0,(float)assets->background.width,(float)assets->background.height},
        (Rectangle){0,0,(float)SCREEN_W,(float)WORLD_AREA_H},
        (Vector2){0,0}, 0.0f, WHITE);

    for (int i = 0; i < HOUSE_DECOR_COUNT; i++) draw_house(&HOUSES[i], assets);
    for (int i = 0; i < count; i++) draw_agent(&agents[i], assets);
    DrawRectangle(0, WORLD_AREA_H, SCREEN_W, 2, DARKGRAY);

    DrawRectangle(0, 0, 430, 28, (Color){0,0,0,100});
    DrawCircle( 10,14,5,(Color){150,150,150,255}); DrawText("Leisure",  20, 7,14,WHITE);
    DrawCircle( 95,14,5,(Color){160,100, 40,255}); DrawText("Chopping",105, 7,14,WHITE);
    DrawCircle(200,14,5,(Color){220,140, 60,255}); DrawText("Building",210, 7,14,WHITE);
    DrawCircle(300,14,5,YELLOW);                   DrawText("Trading", 310, 7,14,WHITE);

    char speedBuf[32]; Color speedCol;
    if      (paused)       { snprintf(speedBuf,sizeof(speedBuf),"PAUSED");              speedCol=RED;    }
    else if (simSteps > 1) { snprintf(speedBuf,sizeof(speedBuf),"Speed: %dx",simSteps); speedCol=ORANGE; }
    else                   { snprintf(speedBuf,sizeof(speedBuf),"Speed: 1x");           speedCol=WHITE;  }
    DrawRectangle(SCREEN_W-200,0,200,42,(Color){0,0,0,100});
    DrawText(speedBuf,SCREEN_W-110,6,16,speedCol);
    DrawText("[SPACE] pause  [F] speed",SCREEN_W-188,26,12,(Color){200,200,200,255});
}

// ---------------------------------------------------------------------------
// Plot helpers
// ---------------------------------------------------------------------------

static Color emv_color(float maxUtility, unsigned char alpha) {
    float t = maxUtility / 150.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (Color){(unsigned char)(55+t*200),30,(unsigned char)(255-t*200),alpha};
}

static void draw_axes_y(int px, int py, int pw, int ph, float yMax, float refLine) {
    DrawLine(px,py,px,py+ph,LIGHTGRAY);
    DrawLine(px,py+ph,px+pw,py+ph,LIGHTGRAY);
    for (int step=0;step<=4;step++) {
        float v=yMax*(float)step/4.0f;
        int y=py+ph-(int)((float)step/4.0f*(float)ph);
        DrawLine(px-4,y,px,y,LIGHTGRAY);
        char buf[12]; snprintf(buf,sizeof(buf),"%.0f",v);
        DrawText(buf,px-28,y-7,11,LIGHTGRAY);
        DrawLine(px,y,px+pw,y,(Color){50,50,60,255});
    }
    if (refLine>0.0f && refLine<=yMax) {
        int ry=py+ph-(int)(refLine/yMax*(float)ph);
        DrawLine(px,ry,px+pw,ry,(Color){255,200,0,110});
        DrawText("equil.",px+pw-46,ry-13,11,(Color){255,200,0,180});
    }
}

static float compute_ymax(float rawMax) {
    return (float)(((int)(rawMax/20)+2)*20);
}

// Clip a line segment to the screen-Y band [yMin, yMax] before drawing.
// Screen Y increases downward: yMin=plot_top, yMax=plot_bottom.
static void draw_line_yclip(int x0, int y0, int x1, int y1,
                             int yMin, int yMax, Color col) {
    // Quick reject: both endpoints on the same side
    if (y0 < yMin && y1 < yMin) return;
    if (y0 > yMax && y1 > yMax) return;

    float t0 = 0.0f, t1 = 1.0f;
    float dy = (float)(y1 - y0);

    if (dy != 0.0f) {
        // Clip start point
        if      (y0 < yMin) t0 = ((float)yMin - (float)y0) / dy;
        else if (y0 > yMax) t0 = ((float)yMax - (float)y0) / dy;
        // Clip end point (parametrize from original P0)
        if      (y1 < yMin) t1 = ((float)yMin - (float)y0) / dy;
        else if (y1 > yMax) t1 = ((float)yMax - (float)y0) / dy;
        if (t0 > t1) { float tmp=t0; t0=t1; t1=tmp; }
    }

    float dx = (float)(x1 - x0);
    DrawLine((int)((float)x0 + t0*dx), (int)((float)y0 + t0*dy),
             (int)((float)x0 + t1*dx), (int)((float)y0 + t1*dy), col);
}

// ---------------------------------------------------------------------------
// PLOT_WEALTH
// ---------------------------------------------------------------------------

static void draw_wealth_panel(const Agent *agents, int count, int marketId,
                               int px, int py, int pw, int ph) {
    PlotBounds *b = &g_bounds[PLOT_WEALTH][marketId];

    float dynMoney=1.0f; int dynGoods=1;
    for (int i=0;i<count;i++) {
        if (agents[i].econ.money>dynMoney) dynMoney=agents[i].econ.money;
        if (agents[i].econ.markets[marketId].goods>dynGoods) dynGoods=agents[i].econ.markets[marketId].goods;
    }
    dynMoney=(float)(((int)(dynMoney/50)+1)*50);
    if (dynGoods%5!=0) dynGoods=(dynGoods/5+1)*5;

    float maxMoney=(b->yMax>0.0f)?b->yMax:dynMoney;
    int   maxGoods=(b->xMax>0.0f)?(int)b->xMax:dynGoods;

    DrawLine(px,py,px,py+ph,LIGHTGRAY);
    DrawLine(px,py+ph,px+pw,py+ph,LIGHTGRAY);
    for (int step=0;step<=4;step++) {
        float v=maxMoney*(float)step/4.0f;
        int y=py+ph-(int)((float)step/4.0f*(float)ph);
        DrawLine(px-4,y,px,y,LIGHTGRAY);
        char buf[12]; snprintf(buf,sizeof(buf),"%.0f",v);
        DrawText(buf,px-28,y-7,11,LIGHTGRAY);
        DrawLine(px,y,px+pw,y,(Color){50,50,60,255});
    }
    for (int step=0;step<=4;step++) {
        int gv=maxGoods*step/4;
        int x=px+(int)((float)step/4.0f*(float)pw);
        DrawLine(x,py+ph,x,py+ph+4,LIGHTGRAY);
        char buf[16]; snprintf(buf,sizeof(buf),"%d",gv);
        DrawText(buf,x-5,py+ph+6,11,LIGHTGRAY);
        DrawLine(x,py,x,py+ph,(Color){50,50,60,255});
    }
    DrawText("$",px-12,py-2,13,LIGHTGRAY);
    DrawText("goods →",px+pw-52,py+ph+6,11,LIGHTGRAY);

    for (int i=0;i<count;i++) {
        float gx=(float)agents[i].econ.markets[marketId].goods/(float)maxGoods;
        float gy=agents[i].econ.money/maxMoney;
        // Skip dots outside plot bounds (no line to draw to edge for scatter)
        if (gx < 0.0f || gx > 1.0f || gy < 0.0f || gy > 1.0f) continue;
        int sx=px+(int)(gx*(float)pw), sy=py+ph-(int)(gy*(float)ph);
        AgentAction act=agents[i].econ.lastAction;
        Color col;
        if      (agents[i].sprite.tradeFlash>0.0f) col=YELLOW;
        else if (act==ACTION_CHOP)          col=(Color){160,100, 40,220};
        else if (act==ACTION_BUILD)         col=(Color){220,140, 60,220};
        else                                col=(Color){150,150,150,180};
        DrawCircle(sx,sy,3,col);
    }
}

// ---------------------------------------------------------------------------
// PLOT_AGENT_VALUES
// ---------------------------------------------------------------------------

static const Agent *s_sort_agents   = NULL;
static int          s_sort_marketId = 0;
static int cmp_by_max_utility(const void *a, const void *b) {
    float fa=s_sort_agents[*(const int*)a].econ.markets[s_sort_marketId].maxUtility;
    float fb=s_sort_agents[*(const int*)b].econ.markets[s_sort_marketId].maxUtility;
    return (fa>fb)-(fa<fb);
}

static void draw_agent_panel(const Agent *agents, int count, int marketId,
                              int px, int py, int pw, int ph, float equilibrium) {
    PlotBounds *b=&g_bounds[PLOT_VALUATION_DISTRIBUTION][marketId];
    float rawMax=1.0f;
    for (int i=0;i<count;i++) {
        const AgentMarket *m=&agents[i].econ.markets[marketId];
        float v=m->maxUtility;
        if (m->priceExpectation>v) v=m->priceExpectation;
        if (v>rawMax) rawMax=v;
    }
    float yMax=(b->yMax>0.0f)?b->yMax:compute_ymax(rawMax);
    draw_axes_y(px,py,pw,ph,yMax,equilibrium);

    int indices[MAX_AGENTS];
    for (int i=0;i<count;i++) indices[i]=i;
    s_sort_agents=agents; s_sort_marketId=marketId;
    qsort(indices,(size_t)count,sizeof(int),cmp_by_max_utility);

    float xStep=(float)pw/(float)(count-1);
    for (int rank=0;rank<count;rank++) {
        const Agent      *a=&agents[indices[rank]];
        const AgentMarket *m=&a->econ.markets[marketId];
        int x=px+(int)((float)rank*xStep);
        float sellPrice=marginal_sell_utility(m), buyPrice=marginal_buy_utility(m);
        float base=m->maxUtility, emv=m->priceExpectation;

        int y_base=(int)(py+ph-base     /yMax*(float)ph);
        int y_sell=(int)(py+ph-sellPrice/yMax*(float)ph);
        int y_buy =(int)(py+ph-buyPrice /yMax*(float)ph);
        int y_emv =(int)(py+ph-emv      /yMax*(float)ph);

        // Connector line clipped to plot area
        draw_line_yclip(x,y_base,x,y_emv, py,py+ph, (Color){80,80,90,120});

        // Hold-zone band clipped to plot area
        int band_top_raw = y_buy<y_sell?y_buy:y_sell;
        int band_bot_raw = (y_sell<y_buy?y_buy:y_sell);
        if (band_bot_raw < band_top_raw+1) band_bot_raw = band_top_raw+1;
        int band_top = band_top_raw < py     ? py     : band_top_raw;
        int band_bot = band_bot_raw > py+ph  ? py+ph  : band_bot_raw;
        if (band_bot > band_top)
            DrawRectangle(x-1,band_top,3,band_bot-band_top,(Color){180,180,100,40});

        // Only draw circles that are within the plot area
        if (y_base>=py && y_base<=py+ph) DrawCircle(x,y_base,3,(Color){ 80,140,255,220});
        if (y_sell>=py && y_sell<=py+ph) DrawCircle(x,y_sell,2,(Color){255,160, 60,200});
        if (y_buy >=py && y_buy <=py+ph) DrawCircle(x,y_buy, 2,(Color){ 80,220,220,200});
        Color emvCol=(a->sprite.tradeFlash>0.0f)?YELLOW
                    :(wants_to_buy(m, a->econ.money) ?(Color){ 60,210, 90,220}
                     :wants_to_sell(m, a->econ.money)?(Color){220, 70, 70,220}
                                                      :(Color){150,150,150,200});
        if (y_emv>=py && y_emv<=py+ph) DrawCircle(x,y_emv,3,emvCol);
    }

    int lx=px+pw-200,ly=py+4;
    DrawCircle(lx,   ly+4, 3,(Color){ 80,140,255,220}); DrawText("Max utility",  lx+7,  ly-1, 10,(Color){ 80,140,255,220});
    DrawCircle(lx,   ly+16,2,(Color){255,160, 60,200}); DrawText("Sell util",    lx+7,  ly+11,10,(Color){255,160, 60,200});
    DrawCircle(lx+100,ly+4, 2,(Color){ 80,220,220,200}); DrawText("Buy util",    lx+107,ly-1, 10,(Color){ 80,220,220,200});
    DrawCircle(lx+100,ly+16,3,(Color){ 60,210, 90,220}); DrawText("Price expect",lx+107,ly+11,10,(Color){ 60,210, 90,220});
}

// ---------------------------------------------------------------------------
// PLOT_EMV_HISTORY
// ---------------------------------------------------------------------------

static void draw_timeseries_panel(const AgentValueHistory *avh,
                                   const AgentValueHistory *pvh,
                                   const Agent *agents, int marketId,
                                   int px, int py, int pw, int ph,
                                   float equilibrium) {
    PlotBounds *b=&g_bounds[PLOT_PRICE_HISTORY][marketId];
    float rawMax=1.0f;
    for (int s=0;s<avh->count;s++) { float v=avh_avg(avh,s); if(v>rawMax) rawMax=v; }
    for (int ag=0;ag<avh->agentCount;ag++) {
        float v=agents[ag].econ.markets[marketId].maxUtility;
        if (v>rawMax) rawMax=v;
    }
    float yMax=(b->yMax>0.0f)?b->yMax:compute_ymax(rawMax);
    draw_axes_y(px,py,pw,ph,yMax,equilibrium);
    if (avh->count<2) return;
    float xScale=(float)pw/(float)(PRICE_HISTORY_SIZE-1);

    if (pvh->count>=2) {
        for (int ag=0;ag<pvh->agentCount;ag++) {
            Color col={50,180,100,30};
            for (int s=1;s<pvh->count;s++) {
                float v0=avh_get(pvh,ag,s-1), v1=avh_get(pvh,ag,s);
                draw_line_yclip(px+(int)((float)(s-1)*xScale),py+ph-(int)(v0/yMax*(float)ph),
                                px+(int)((float)s    *xScale),py+ph-(int)(v1/yMax*(float)ph),
                                py,py+ph,col);
            }
        }
        for (int s=1;s<pvh->count;s++) {
            float v0=avh_avg(pvh,s-1), v1=avh_avg(pvh,s);
            draw_line_yclip(px+(int)((float)(s-1)*xScale),py+ph-(int)(v0/yMax*(float)ph),
                            px+(int)((float)s    *xScale),py+ph-(int)(v1/yMax*(float)ph),
                            py,py+ph,(Color){80,230,130,230});
        }
    }
    for (int ag=0;ag<avh->agentCount;ag++) {
        Color col=emv_color(agents[ag].econ.markets[marketId].maxUtility,55);
        for (int s=1;s<avh->count;s++) {
            float v0=avh_get(avh,ag,s-1), v1=avh_get(avh,ag,s);
            draw_line_yclip(px+(int)((float)(s-1)*xScale),py+ph-(int)(v0/yMax*(float)ph),
                            px+(int)((float)s    *xScale),py+ph-(int)(v1/yMax*(float)ph),
                            py,py+ph,col);
        }
    }
    for (int s=1;s<avh->count;s++) {
        float v0=avh_avg(avh,s-1), v1=avh_avg(avh,s);
        draw_line_yclip(px+(int)((float)(s-1)*xScale),py+ph-(int)(v0/yMax*(float)ph),
                        px+(int)((float)s    *xScale),py+ph-(int)(v1/yMax*(float)ph),
                        py,py+ph,WHITE);
    }

    int lx=px+pw-200,ly=py+4;
    DrawLine(lx,   ly+4, lx+12, ly+4, emv_color(50.0f,200));     DrawText("Price/agent",lx+16,ly-1, 10,(Color){200,200,200,255});
    DrawLine(lx+100,ly+4, lx+112,ly+4, (Color){50,180,100,200}); DrawText("Util/agent", lx+116,ly-1, 10,(Color){80,210,130,255});
    DrawLine(lx,   ly+15,lx+12, ly+15,WHITE);                    DrawText("Price avg",  lx+16, ly+10,10,WHITE);
    DrawLine(lx+100,ly+15,lx+112,ly+15,(Color){80,230,130,230}); DrawText("Util avg",   lx+116,ly+10,10,(Color){80,230,130,255});
}

// ---------------------------------------------------------------------------
// PLOT_GOODS_HISTORY
// ---------------------------------------------------------------------------

static void draw_goods_panel(const AgentValueHistory *gvh,
                              int marketId,
                              int px, int py, int pw, int ph) {
    PlotBounds *b = &g_bounds[PLOT_GOODS_HISTORY][marketId];
    float rawMax = 1.0f;
    for (int s = 0; s < gvh->count; s++) {
        float v = avh_avg(gvh, s);
        if (v > rawMax) rawMax = v;
    }
    for (int ag = 0; ag < gvh->agentCount; ag++) {
        for (int s = 0; s < gvh->count; s++) {
            float v = avh_get(gvh, ag, s);
            if (v > rawMax) rawMax = v;
        }
    }
    float yMax = (b->yMax > 0.0f) ? b->yMax : compute_ymax(rawMax);
    draw_axes_y(px, py, pw, ph, yMax, 0.0f);
    if (gvh->count < 2) return;

    float xScale = (float)pw / (float)(PRICE_HISTORY_SIZE - 1);

    // Individual agent lines (dim)
    for (int ag = 0; ag < gvh->agentCount; ag++) {
        Color col = {80, 160, 220, 35};
        for (int s = 1; s < gvh->count; s++) {
            float v0 = avh_get(gvh, ag, s-1), v1 = avh_get(gvh, ag, s);
            draw_line_yclip(px + (int)((float)(s-1) * xScale), py + ph - (int)(v0 / yMax * (float)ph),
                            px + (int)((float) s    * xScale), py + ph - (int)(v1 / yMax * (float)ph),
                            py, py + ph, col);
        }
    }
    // Average line (bright)
    for (int s = 1; s < gvh->count; s++) {
        float v0 = avh_avg(gvh, s-1), v1 = avh_avg(gvh, s);
        draw_line_yclip(px + (int)((float)(s-1) * xScale), py + ph - (int)(v0 / yMax * (float)ph),
                        px + (int)((float) s    * xScale), py + ph - (int)(v1 / yMax * (float)ph),
                        py, py + ph, WHITE);
    }

    int lx = px + pw - 160, ly = py + 4;
    DrawLine(lx, ly+4, lx+12, ly+4, (Color){80,160,220,180});
    DrawText("goods/agent", lx+16, ly-1, 10, (Color){160,200,220,255});
    DrawLine(lx, ly+15, lx+12, ly+15, WHITE);
    DrawText("avg goods",   lx+16, ly+10, 10, WHITE);
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
            draw_wealth_panel(agents,agentCount,mid,px,py,pw,ph); break;
        case PLOT_VALUATION_DISTRIBUTION:
            draw_agent_panel(agents,agentCount,mid,px,py,pw,ph,equilibrium); break;
        case PLOT_PRICE_HISTORY:
            draw_timeseries_panel(&avh[mid],&pvh[mid],agents,mid,px,py,pw,ph,equilibrium); break;
        case PLOT_GOODS_HISTORY:
            draw_goods_panel(&gvh[mid],mid,px,py,pw,ph); break;
        default: break;
    }
}

// Compute panel geometry for a given panel index (0=TL,1=TR,2=BL,3=BR)
static void panel_geom(int panelIdx,
                       int *out_px, int *out_strip_y, int *out_py, int *out_ph) {
    int half     = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int px_col[2] = { PLOT_MARGIN_L, PLOT_MARGIN_L + half + PANEL_GAP };
    int plots_y  = WORLD_AREA_H + 2;
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

void render_plot(const AgentValueHistory avh[MARKET_COUNT],
                 const AgentValueHistory pvh[MARKET_COUNT],
                 const AgentValueHistory gvh[MARKET_COUNT],
                 const Agent *agents, int agentCount,
                 PanelState panels[NUM_PANELS]) {
    int half     = (SCREEN_W - PLOT_MARGIN_L - PLOT_MARGIN_R - PANEL_GAP) / 2;
    int plots_y  = WORLD_AREA_H + 2;
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
