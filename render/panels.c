#include "render/panels.h"
#include "econ/market.h"
#include "econ/econ.h"
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Shared plot bounds (non-static so render.c can share via extern in panels.h)
// ---------------------------------------------------------------------------
PlotBounds g_bounds[PLOT_COUNT][MARKET_COUNT];

// ---------------------------------------------------------------------------
// Helpers
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
        DrawTextF(buf,px-28,y-7,11,LIGHTGRAY);
        DrawLine(px,y,px+pw,y,(Color){50,50,60,255});
    }
    if (refLine>0.0f && refLine<=yMax) {
        int ry=py+ph-(int)(refLine/yMax*(float)ph);
        DrawLine(px,ry,px+pw,ry,(Color){255,200,0,110});
        DrawTextF("equil.",px+pw-46,ry-13,11,(Color){255,200,0,180});
    }
}

static float compute_ymax(float rawMax) {
    return (float)(((int)(rawMax/20)+2)*20);
}

static void draw_line_yclip(int x0, int y0, int x1, int y1,
                             int yMin, int yMax, Color col) {
    if (y0 < yMin && y1 < yMin) return;
    if (y0 > yMax && y1 > yMax) return;

    float t0 = 0.0f, t1 = 1.0f;
    float dy = (float)(y1 - y0);

    if (dy != 0.0f) {
        if      (y0 < yMin) t0 = ((float)yMin - (float)y0) / dy;
        else if (y0 > yMax) t0 = ((float)yMax - (float)y0) / dy;
        if      (y1 < yMin) t1 = ((float)yMin - (float)y0) / dy;
        else if (y1 > yMax) t1 = ((float)yMax - (float)y0) / dy;
        if (t0 > t1) { float tmp=t0; t0=t1; t1=tmp; }
    }

    float dx = (float)(x1 - x0);
    DrawLine((int)((float)x0 + t0*dx), (int)((float)y0 + t0*dy),
             (int)((float)x0 + t1*dx), (int)((float)y0 + t1*dy), col);
}

static int cmp_float_desc(const void *a, const void *b) {
    float fa = *(const float*)a, fb = *(const float*)b;
    return (fa < fb) - (fa > fb);
}
static int cmp_float_asc(const void *a, const void *b) {
    float fa = *(const float*)a, fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

static const Agent *s_sort_agents   = NULL;
static int          s_sort_marketId = 0;
static int cmp_by_max_utility(const void *a, const void *b) {
    float fa=s_sort_agents[*(const int*)a].econ.markets[s_sort_marketId].maxUtility;
    float fb=s_sort_agents[*(const int*)b].econ.markets[s_sort_marketId].maxUtility;
    return (fa>fb)-(fa<fb);
}

// ---------------------------------------------------------------------------
// PLOT_WEALTH
// ---------------------------------------------------------------------------

void panel_wealth(const Agent *agents, int count, int marketId,
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
        DrawTextF(buf,px-28,y-7,11,LIGHTGRAY);
        DrawLine(px,y,px+pw,y,(Color){50,50,60,255});
    }
    for (int step=0;step<=4;step++) {
        int gv=maxGoods*step/4;
        int x=px+(int)((float)step/4.0f*(float)pw);
        DrawLine(x,py+ph,x,py+ph+4,LIGHTGRAY);
        char buf[16]; snprintf(buf,sizeof(buf),"%d",gv);
        DrawTextF(buf,x-5,py+ph+6,11,LIGHTGRAY);
        DrawLine(x,py,x,py+ph,(Color){50,50,60,255});
    }
    DrawTextF("$",px-12,py-2,13,LIGHTGRAY);
    DrawTextF("goods →",px+pw-52,py+ph+6,11,LIGHTGRAY);

    for (int i=0;i<count;i++) {
        float gx=(float)agents[i].econ.markets[marketId].goods/(float)maxGoods;
        float gy=agents[i].econ.money/maxMoney;
        AgentAction act=agents[i].econ.lastAction;
        Color col;
        if      (agents[i].sprite.tradeFlashTick>0) col=YELLOW;
        else if (act==ACTION_CHOP)          col=(Color){160,100, 40,220};
        else if (act==ACTION_BUILD)         col=(Color){220,140, 60,220};
        else                                col=(Color){150,150,150,180};

        if (gx >= 0.0f && gx <= 1.0f && gy >= 0.0f && gy <= 1.0f) {
            int sx=px+(int)(gx*(float)pw), sy=py+ph-(int)(gy*(float)ph);
            DrawCircle(sx,sy,3,col);
        } else {
            // Compute actual pixel position (may be outside plot)
            float ax = (float)px + gx*(float)pw;
            float ay = (float)(py+ph) - gy*(float)ph;
            // Clamp to plot border
            float ex = ax < (float)px ? (float)px : (ax > (float)(px+pw) ? (float)(px+pw) : ax);
            float ey = ay < (float)py ? (float)py : (ay > (float)(py+ph) ? (float)(py+ph) : ay);
            // Direction from clamped point toward actual point (screen coords, y-down)
            float ddx = ax - ex, ddy = ay - ey;
            float len = sqrtf(ddx*ddx + ddy*ddy);
            if (len < 0.5f) continue;
            ddx /= len; ddy /= len;
            // Triangle pointing in direction (ddx,ddy), tip 6px out, base 4px wide
            // Winding: DrawTriangle(tip, v_right, v_left) is CCW in screen y-down
            Vector2 tip = {ex + ddx*6.0f,          ey + ddy*6.0f         };
            Vector2 vr  = {ex - ddx + ddy*4.0f,    ey - ddy - ddx*4.0f  };
            Vector2 vl  = {ex - ddx - ddy*4.0f,    ey - ddy + ddx*4.0f  };
            DrawTriangle(tip, vr, vl, col);
        }
    }
}

// ---------------------------------------------------------------------------
// PLOT_VALUATION_DISTRIBUTION
// ---------------------------------------------------------------------------

void panel_valuation_dist(const Agent *agents, int count, int marketId,
                           int px, int py, int pw, int ph, Price equilibrium) {
    PlotBounds *b=&g_bounds[PLOT_VALUATION_DISTRIBUTION][marketId];
    float rawMax=1.0f;
    for (int i=0;i<count;i++) {
        const AgentMarket *m=&agents[i].econ.markets[marketId];
        float v=m->maxUtility;
        if (m->priceExpectation>v) v=m->priceExpectation;
        if (v>rawMax) rawMax=v;
    }
    // When inflation is on, also account for inflation-adjusted effective prices in scaling
    if (g_inflation_enabled) {
        for (int i=0;i<count;i++) {
            const AgentMarket *m=&agents[i].econ.markets[marketId];
            Utility mmu=money_marginal_utility(agents[i].econ.money);
            float v=m->maxUtility/mmu;
            if (v>rawMax) rawMax=v;
        }
    }
    float yMax=(b->yMax>0.0f)?b->yMax:compute_ymax(rawMax);
    draw_axes_y(px,py,pw,ph,yMax,equilibrium);

    int indices[MAX_AGENTS];
    for (int i=0;i<count;i++) indices[i]=i;
    s_sort_agents=agents; s_sort_marketId=marketId;
    qsort(indices,(size_t)count,sizeof(int),cmp_by_max_utility);

    float xStep=(float)pw/(float)(count);
    float xOff=xStep*0.5f;
    for (int rank=0;rank<count;rank++) {
        const Agent      *a=&agents[indices[rank]];
        const AgentMarket *m=&a->econ.markets[marketId];
        int x=px+(int)(xOff+(float)rank*xStep);
        Utility sellUtility=marginal_sell_utility(m), buyUtility=marginal_buy_utility(m);
        Utility base=m->maxUtility; Price emv=m->priceExpectation;

        int y_base=(int)(py+ph-base        /yMax*(float)ph);
        int y_sell=(int)(py+ph-sellUtility /yMax*(float)ph);
        int y_buy =(int)(py+ph-buyUtility  /yMax*(float)ph);
        int y_emv =(int)(py+ph-emv         /yMax*(float)ph);

        draw_line_yclip(x,y_base,x,y_emv, py,py+ph, (Color){80,80,90,120});

        int band_top_raw = y_buy<y_sell?y_buy:y_sell;
        int band_bot_raw = (y_sell<y_buy?y_buy:y_sell);
        if (band_bot_raw < band_top_raw+1) band_bot_raw = band_top_raw+1;
        int band_top = band_top_raw < py     ? py     : band_top_raw;
        int band_bot = band_bot_raw > py+ph  ? py+ph  : band_bot_raw;
        if (band_bot > band_top)
            DrawRectangle(x-1,band_top,3,band_bot-band_top,(Color){180,180,100,40});

        if (g_inflation_enabled) {
            // Only show utility base when inflation is active (buy/sell shown via adjusted dots)
            if (y_base>=py && y_base<=py+ph) DrawCircle(x,y_base,2,(Color){ 80,140,255,130});

            // Inflation-adjusted effective prices: utility / money_marginal_utility
            Utility mmu=money_marginal_utility(a->econ.money);
            Price adj_base=base       /mmu;
            Price adj_sell=sellUtility/mmu;
            Price adj_buy =buyUtility /mmu;
            int y_adj_base=(int)(py+ph-adj_base/yMax*(float)ph);
            int y_adj_sell=(int)(py+ph-adj_sell/yMax*(float)ph);
            int y_adj_buy =(int)(py+ph-adj_buy /yMax*(float)ph);
            if (y_adj_base>=py && y_adj_base<=py+ph) DrawCircle(x,y_adj_base,3,(Color){140,190,255,255});
            if (y_adj_sell>=py && y_adj_sell<=py+ph) DrawCircle(x,y_adj_sell,2,(Color){255,200,100,255});
            if (y_adj_buy >=py && y_adj_buy <=py+ph) DrawCircle(x,y_adj_buy, 2,(Color){100,240,240,255});
        } else {
            if (y_base>=py && y_base<=py+ph) DrawCircle(x,y_base,3,(Color){ 80,140,255,220});
            if (y_sell>=py && y_sell<=py+ph) DrawCircle(x,y_sell,2,(Color){255,160, 60,200});
            if (y_buy >=py && y_buy <=py+ph) DrawCircle(x,y_buy, 2,(Color){ 80,220,220,200});
        }

        Color emvCol=(a->sprite.tradeFlashTick>0)?YELLOW
                    :(is_buyer(a, (MarketId)marketId) ?(Color){ 60,210, 90,220}
                     :is_seller(a, (MarketId)marketId)?(Color){220, 70, 70,220}
                                                       :(Color){150,150,150,200});
        if (y_emv>=py && y_emv<=py+ph) DrawCircle(x,y_emv,3,emvCol);
    }

    int lx=px+pw-200,ly=py+4;
    if (g_inflation_enabled) {
        // Left column: raw utility dots (dimmed); right column: inflation-adjusted effective prices
        DrawCircle(lx,    ly+4,  2,(Color){ 80,140,255,130}); DrawTextF("Max util", lx+7,   ly-1,  10,(Color){ 80,140,255,130});
        DrawCircle(lx,    ly+16, 3,(Color){ 60,210, 90,220}); DrawTextF("Price exp",lx+7,   ly+11, 10,(Color){ 60,210, 90,220});
        DrawCircle(lx+100,ly+4,  3,(Color){140,190,255,255}); DrawTextF("Adj. base",lx+107, ly-1,  10,(Color){140,190,255,255});
        DrawCircle(lx+100,ly+16, 2,(Color){255,200,100,255}); DrawTextF("Adj. sell",lx+107, ly+11, 10,(Color){255,200,100,255});
        DrawCircle(lx+100,ly+28, 2,(Color){100,240,240,255}); DrawTextF("Adj. buy", lx+107, ly+23, 10,(Color){100,240,240,255});
    } else {
        DrawCircle(lx,    ly+4,  3,(Color){ 80,140,255,220}); DrawTextF("Max utility",  lx+7,   ly-1,  10,(Color){ 80,140,255,220});
        DrawCircle(lx,    ly+16, 2,(Color){255,160, 60,200}); DrawTextF("Sell util",    lx+7,   ly+11, 10,(Color){255,160, 60,200});
        DrawCircle(lx+100,ly+4,  2,(Color){ 80,220,220,200}); DrawTextF("Buy util",     lx+107, ly-1,  10,(Color){ 80,220,220,200});
        DrawCircle(lx+100,ly+16, 3,(Color){ 60,210, 90,220}); DrawTextF("Price expect", lx+107, ly+11, 10,(Color){ 60,210, 90,220});
    }
}

// ---------------------------------------------------------------------------
// PLOT_PRICE_HISTORY
// ---------------------------------------------------------------------------

void panel_price_history(const AgentValueHistory *avh, const AgentValueHistory *pvh,
                         const Agent *agents, int count, int marketId,
                         int px, int py, int pw, int ph,
                         Price equilibrium, bool showIndividualUtil) {
    PlotBounds *b=&g_bounds[PLOT_PRICE_HISTORY][marketId];
    float rawMax=1.0f;
    for (int s=0;s<avh->count;s++) { float v=avh_avg(avh,s); if(v>rawMax) rawMax=v; }
    for (int ag=0;ag<count;ag++) {
        float v=agents[ag].econ.markets[marketId].maxUtility;
        if (v>rawMax) rawMax=v;
    }
    float yMax=(b->yMax>0.0f)?b->yMax:compute_ymax(rawMax);
    draw_axes_y(px,py,pw,ph,yMax,equilibrium);
    if (avh->count<2) return;
    float xScale=(float)pw/(float)(PRICE_HISTORY_SIZE-1);

    if (showIndividualUtil && pvh->count>=2) {
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
    DrawLine(lx,   ly+4, lx+12, ly+4, emv_color(50.0f,200));     DrawTextF("Price/agent",lx+16,ly-1, 10,(Color){200,200,200,255});
    if (showIndividualUtil) {
        DrawLine(lx+100,ly+4, lx+112,ly+4, (Color){50,180,100,200}); DrawTextF("Util/agent", lx+116,ly-1, 10,(Color){80,210,130,255});
    }
    DrawLine(lx,   ly+15,lx+12, ly+15,WHITE);                    DrawTextF("Price avg",  lx+16, ly+10,10,WHITE);
    if (showIndividualUtil) {
        DrawLine(lx+100,ly+15,lx+112,ly+15,(Color){80,230,130,230}); DrawTextF("Util avg",   lx+116,ly+10,10,(Color){80,230,130,255});
    }
}

// ---------------------------------------------------------------------------
// PLOT_GOODS_HISTORY
// ---------------------------------------------------------------------------

void panel_goods_history(const AgentValueHistory *gvh, int marketId,
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

    for (int ag = 0; ag < gvh->agentCount; ag++) {
        Color col = {80, 160, 220, 35};
        for (int s = 1; s < gvh->count; s++) {
            float v0 = avh_get(gvh, ag, s-1), v1 = avh_get(gvh, ag, s);
            draw_line_yclip(px + (int)((float)(s-1) * xScale), py + ph - (int)(v0 / yMax * (float)ph),
                            px + (int)((float) s    * xScale), py + ph - (int)(v1 / yMax * (float)ph),
                            py, py + ph, col);
        }
    }
    for (int s = 1; s < gvh->count; s++) {
        float v0 = avh_avg(gvh, s-1), v1 = avh_avg(gvh, s);
        draw_line_yclip(px + (int)((float)(s-1) * xScale), py + ph - (int)(v0 / yMax * (float)ph),
                        px + (int)((float) s    * xScale), py + ph - (int)(v1 / yMax * (float)ph),
                        py, py + ph, WHITE);
    }

    int lx = px + pw - 160, ly = py + 4;
    DrawLine(lx, ly+4, lx+12, ly+4, (Color){80,160,220,180});
    DrawTextF("goods/agent", lx+16, ly-1, 10, (Color){160,200,220,255});
    DrawLine(lx, ly+15, lx+12, ly+15, WHITE);
    DrawTextF("avg goods",   lx+16, ly+10, 10, WHITE);
}

// ---------------------------------------------------------------------------
// PLOT_SUPPLY_DEMAND
// ---------------------------------------------------------------------------

void panel_supply_demand(const Agent *agents, int count, int marketId,
                         int px, int py, int pw, int ph) {
    PlotBounds *b = &g_bounds[PLOT_SUPPLY_DEMAND][marketId];

    float demand[MAX_AGENTS];
    int nDemand = 0;
    for (int i = 0; i < count; i++)
        demand[nDemand++] = marginal_buy_utility(&agents[i].econ.markets[marketId]);
    qsort(demand, (size_t)nDemand, sizeof(float), cmp_float_desc);

    float supply[MAX_AGENTS];
    int nSupply = 0;
    for (int i = 0; i < count; i++) {
        const AgentMarket *m = &agents[i].econ.markets[marketId];
        if (m->goods > 0)
            supply[nSupply++] = marginal_sell_utility(m);
    }
    qsort(supply, (size_t)nSupply, sizeof(float), cmp_float_asc);

    float rawMax = 1.0f;
    for (int i = 0; i < nDemand; i++) if (demand[i] > rawMax) rawMax = demand[i];
    for (int i = 0; i < nSupply; i++) if (supply[i] > rawMax) rawMax = supply[i];
    float yMax = (b->yMax > 0.0f) ? b->yMax : compute_ymax(rawMax);

    float eqPrice = 0.0f;
    int   eqQty   = 0;
    {
        int n = nDemand < nSupply ? nDemand : nSupply;
        for (int i = 0; i < n; i++) {
            if (demand[i] >= supply[i]) {
                eqPrice = (demand[i] + supply[i]) * 0.5f;
                eqQty   = i + 1;
            } else break;
        }
    }

    draw_axes_y(px, py, pw, ph, yMax, eqPrice);

    for (int step = 0; step <= 4; step++) {
        int gv = count * step / 4;
        int x  = px + (int)((float)step / 4.0f * (float)pw);
        DrawLine(x, py+ph, x, py+ph+4, LIGHTGRAY);
        char buf[16]; snprintf(buf, sizeof(buf), "%d", gv);
        DrawTextF(buf, x-5, py+ph+6, 11, LIGHTGRAY);
        DrawLine(x, py, x, py+ph, (Color){50,50,60,255});
    }
    DrawTextF("agents →", px+pw-56, py+ph+6, 11, LIGHTGRAY);

    Color demandCol = {80, 180, 255, 220};
    for (int i = 0; i < nDemand; i++) {
        int x0 = px + (int)((float) i      / (float)count * (float)pw);
        int x1 = px + (int)((float)(i + 1) / (float)count * (float)pw);
        int y  = py + ph - (int)(demand[i] / yMax * (float)ph);
        if (y < py) y = py;
        if (y > py+ph) y = py+ph;
        DrawLine(x0, y, x1, y, demandCol);
        if (i + 1 < nDemand) {
            int y1 = py + ph - (int)(demand[i+1] / yMax * (float)ph);
            if (y1 < py) y1 = py;
            if (y1 > py+ph) y1 = py+ph;
            DrawLine(x1, y, x1, y1, demandCol);
        }
    }

    Color supplyCol = {255, 110, 80, 220};
    for (int i = 0; i < nSupply; i++) {
        int x0 = px + (int)((float) i      / (float)count * (float)pw);
        int x1 = px + (int)((float)(i + 1) / (float)count * (float)pw);
        int y  = py + ph - (int)(supply[i] / yMax * (float)ph);
        if (y < py) y = py;
        if (y > py+ph) y = py+ph;
        DrawLine(x0, y, x1, y, supplyCol);
        if (i + 1 < nSupply) {
            int y1 = py + ph - (int)(supply[i+1] / yMax * (float)ph);
            if (y1 < py) y1 = py;
            if (y1 > py+ph) y1 = py+ph;
            DrawLine(x1, y, x1, y1, supplyCol);
        }
    }

    if (eqQty > 0) {
        int eqX = px + (int)((float)eqQty / (float)count * (float)pw);
        DrawLine(eqX, py, eqX, py+ph, (Color){255,200,0,70});
        char buf[16]; snprintf(buf, sizeof(buf), "Q*=%d", eqQty);
        DrawTextF(buf, eqX+3, py+4, 10, (Color){255,200,0,200});
    }

    int lx = px + pw - 160, ly = py + 4;
    DrawLine(lx,    ly+4,  lx+12, ly+4,  demandCol); DrawTextF("Demand (buy util)",  lx+16, ly-1,  10, demandCol);
    DrawLine(lx,    ly+16, lx+12, ly+16, supplyCol); DrawTextF("Supply (sell util)",  lx+16, ly+11, 10, supplyCol);
}

// ---------------------------------------------------------------------------
// PLOT_MONEY_HISTORY
// ---------------------------------------------------------------------------

void panel_money_history(const AgentValueHistory *mvh, int marketId,
                         int px, int py, int pw, int ph) {
    PlotBounds *b = &g_bounds[PLOT_MONEY_HISTORY][marketId];
    float rawMax = 1.0f;
    for (int s = 0; s < mvh->count; s++) {
        float v = avh_avg(mvh, s);
        if (v > rawMax) rawMax = v;
    }
    for (int ag = 0; ag < mvh->agentCount; ag++) {
        for (int s = 0; s < mvh->count; s++) {
            float v = avh_get(mvh, ag, s);
            if (v > rawMax) rawMax = v;
        }
    }
    float yMax = (b->yMax > 0.0f) ? b->yMax : compute_ymax(rawMax);
    draw_axes_y(px, py, pw, ph, yMax, 0.0f);
    if (mvh->count < 2) return;

    float xScale = (float)pw / (float)(PRICE_HISTORY_SIZE - 1);

    for (int ag = 0; ag < mvh->agentCount; ag++) {
        Color col = {100, 200, 120, 35};
        for (int s = 1; s < mvh->count; s++) {
            float v0 = avh_get(mvh, ag, s-1), v1 = avh_get(mvh, ag, s);
            draw_line_yclip(px + (int)((float)(s-1) * xScale), py + ph - (int)(v0 / yMax * (float)ph),
                            px + (int)((float) s    * xScale), py + ph - (int)(v1 / yMax * (float)ph),
                            py, py + ph, col);
        }
    }
    for (int s = 1; s < mvh->count; s++) {
        float v0 = avh_avg(mvh, s-1), v1 = avh_avg(mvh, s);
        draw_line_yclip(px + (int)((float)(s-1) * xScale), py + ph - (int)(v0 / yMax * (float)ph),
                        px + (int)((float) s    * xScale), py + ph - (int)(v1 / yMax * (float)ph),
                        py, py + ph, WHITE);
    }

    int lx = px + pw - 160, ly = py + 4;
    DrawLine(lx, ly+4, lx+12, ly+4, (Color){100,200,120,180});
    DrawTextF("money/agent", lx+16, ly-1, 10, (Color){160,220,180,255});
    DrawLine(lx, ly+15, lx+12, ly+15, WHITE);
    DrawTextF("avg money",   lx+16, ly+10, 10, WHITE);
}

// ---------------------------------------------------------------------------
// Control panel forwarders
// ---------------------------------------------------------------------------

void panel_ctrl_influence_render(InfluencePanel *p, Agent *agents, int count, int px, int py) {
    (void)px; (void)py;
    influence_panel_render(p, agents, count);
}
void panel_ctrl_decay_render(DecayRatePanel *p, int px, int py) {
    (void)px; (void)py;
    decay_rate_panel_render(p);
}
