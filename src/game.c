#include "game.h"
Game game;
static int abs_i(int a) { return a<0 ? -a : a; }
static int min_i(int a,int b) { return a<b ? a:b; }
static unsigned random_n(unsigned n) {
    game.rng=game.rng*1664525u+1013904223u;
    return (game.rng>>8)%n;
}
static void message(int m) { game.message=m; game.messageTimer=90; }
int game_tile(int x,int y) {
    if(x<0 || y<0 || x>=WORLD_W || y>=WORLD_H) return WATER;
    return game.map[y][x];
}
static int walkable(int x,int y) {
    int t=game_tile(x,y);
    return t==GRASS || t==SHELTER;
}
int game_warm(int x,int y) {
    if(game_tile(x,y)==SHELTER) return 1;
    for(int j=-3;j<=3;j++) for(int i=-3;i<=3;i++)
        if(abs_i(i)+abs_i(j)<=3 && game_tile(x+i,y+j)==FIRE) return 1;
    return 0;
}
void game_new(u32 seed) {
    /* Explicit byte clearing also works in a freestanding GBA build. */
    u8 *bytes=(u8*)&game;
    for(unsigned i=0;i<sizeof(game);i++) bytes[i]=0;
    game.rng=seed;
    for(int y=0;y<WORLD_H;y++) for(int x=0;x<WORLD_W;x++) {
        unsigned roll=random_n(100);
        int edge=x<2 || y<2 || x>=WORLD_W-2 || y>=WORLD_H-2;
        game.map[y][x]=edge ? WATER : roll<17 ? TREE : roll<24 ? ROCK : roll<30 ? BERRY : GRASS;
    }
    game.x=20; game.y=16; game.dx=1;
    for(int y=13;y<=19;y++) for(int x=17;x<=23;x++) game.map[y][x]=GRASS;
    game.map[16][24]=TREE; game.map[15][24]=TREE; game.map[14][24]=TREE;
    game.map[18][24]=ROCK; game.map[19][24]=ROCK;
    game.map[13][19]=BERRY; game.map[13][20]=BERRY; game.map[13][21]=BERRY;
    game.health=100; game.hunger=85; game.wood=8; game.stone=4; game.food=3;
    game.state=PLAY; game.recipe=1;
    message(WELCOME);
}
static void gather(void) {
    int x=game.x+game.dx,y=game.y+game.dy;
    for(int i=0;i<WOLVES;i++) {
        Wolf *w=&game.wolves[i];
        if(w->hp && abs_i(w->x-game.x)+abs_i(w->y-game.y)<=1) {
            w->hp=0; game.food=min_i(99,game.food+1); message(WOLF_HIT); return;
        }
    }
    switch(game_tile(x,y)) {
    case TREE: game.wood=min_i(99,game.wood+3); message(WOOD_GAIN); break;
    case ROCK: game.stone=min_i(99,game.stone+2); message(STONE_GAIN); break;
    case BERRY: game.food=min_i(99,game.food+2); message(FOOD_GAIN); break;
    case WALL: game.wood=min_i(99,game.wood+1); message(REMOVED); break;
    case FIRE: game.wood=min_i(99,game.wood+2); game.stone=min_i(99,game.stone+1); message(REMOVED); break;
    case SHELTER: game.wood=min_i(99,game.wood+3); game.stone=min_i(99,game.stone+1); message(REMOVED); break;
    default: message(NOTHING); return;
    }
    game.map[y][x]=GRASS;
}
static void build(void) {
    static const int woodCost[]={2,4,6},stoneCost[]={0,2,2};
    int x=game.x+game.dx,y=game.y+game.dy,r=game.recipe;
    if(game_tile(x,y)!=GRASS) { message(BLOCKED); return; }
    for(int i=0;i<WOLVES;i++) if(game.wolves[i].hp && game.wolves[i].x==x && game.wolves[i].y==y) {
        message(BLOCKED); return;
    }
    if(game.wood<woodCost[r] || game.stone<stoneCost[r]) { message(NEED_RESOURCES); return; }
    game.wood-=woodCost[r]; game.stone-=stoneCost[r];
    game.map[y][x]=(u8)(WALL+r); message(BUILT);
}
static void spawn_wolves(void) {
    for(int i=0;i<WOLVES;i++) {
        Wolf *w=&game.wolves[i]; w->hp=0;
        if(i>game.nights+1) continue;
        for(int attempt=0;attempt<160;attempt++) {
            int x=2+(int)random_n(WORLD_W-4),y=2+(int)random_n(WORLD_H-4);
            int d=abs_i(x-game.x)+abs_i(y-game.y);
            if(walkable(x,y) && !game_warm(x,y) && d>=7 && d<18) {
                w->x=x; w->y=y; w->hp=1; break;
            }
        }
    }
}
static void wolves_update(void) {
    for(int i=0;i<WOLVES;i++) {
        Wolf *w=&game.wolves[i];
        if(!w->hp) continue;
        int distance=abs_i(w->x-game.x)+abs_i(w->y-game.y);
        if(distance<=1) {
            if(!game_warm(game.x,game.y) && !game.hitTimer) {
                game.health-=8; game.hitTimer=40; message(ATTACKED);
            }
            continue;
        }
        int dx=(game.x>w->x)-(game.x<w->x),dy=(game.y>w->y)-(game.y<w->y);
        /* Try both axes so a single tree does not permanently trap a wolf. */
        if(random_n(2)) { int t=dx; dx=0; if(!dy) dx=t; }
        else if(dx) dy=0;
        int nx=w->x+dx,ny=w->y+dy;
        if(walkable(nx,ny) && !game_warm(nx,ny) && !(nx==game.x && ny==game.y)) {
            w->x=nx; w->y=ny;
        }
    }
}
void game_update(unsigned held,unsigned pressed) {
    if(game.state!=PLAY) return;
    if(pressed&START) { game.state=PAUSE; return; }
    game.ticks++;
    if(game.messageTimer) game.messageTimer--;
    if(game.hitTimer) game.hitTimer--;
    if(pressed&R) game.recipe=(game.recipe+1)%3;
    if(pressed&L) game.recipe=(game.recipe+2)%3;
    if(game.moveTimer) game.moveTimer--;
    if(!(held&(UP|DOWN|LEFT|RIGHT))) game.moveTimer=0;
    if(!game.moveTimer && (held&(UP|DOWN|LEFT|RIGHT))) {
        game.dx=0; game.dy=0;
        if(held&RIGHT) game.dx=1; else if(held&LEFT) game.dx=-1;
        else if(held&UP) game.dy=-1; else game.dy=1;
        int nx=game.x+game.dx,ny=game.y+game.dy,occupied=0;
        for(int i=0;i<WOLVES;i++) if(game.wolves[i].hp && game.wolves[i].x==nx && game.wolves[i].y==ny) occupied=1;
        if(walkable(nx,ny) && !occupied) { game.x=nx; game.y=ny; }
        game.moveTimer=5;
    }
    if(pressed&A) gather();
    if(pressed&B) build();
    if(pressed&SELECT) {
        if(game.food) { game.food--; game.hunger=min_i(100,game.hunger+30); game.health=min_i(100,game.health+8); message(EATEN); }
        else message(NO_FOOD);
    }
    if(game.ticks%60==0) {
        if(game.hunger>0) game.hunger--; else game.health-=3;
    }
    if(game.night && game.ticks%120==0 && !game_warm(game.x,game.y)) { game.health-=4; message(COLD); }
    if(game.ticks%150==0 && game.hunger>50 && game_warm(game.x,game.y)) game.health=min_i(100,game.health+2);
    if(game.night && game.ticks%24==0) wolves_update();
    if(game.health<=0) { game.health=0; game.state=LOST; return; }
    if(++game.phase >= (game.night ? NIGHT_TICKS : DAY_TICKS)) {
        game.phase=0; game.night=!game.night;
        if(game.night) { spawn_wolves(); message(NIGHTFALL); }
        else {
            for(int i=0;i<WOLVES;i++) game.wolves[i].hp=0;
            if(++game.nights>=3) { game.state=WON; return; }
            /* Renew a little food each morning on unoccupied grass. */
            for(int i=0;i<24;i++) {
                int x=2+(int)random_n(WORLD_W-4),y=2+(int)random_n(WORLD_H-4);
                if(game.map[y][x]==GRASS && !(game.x==x && game.y==y)) game.map[y][x]=BERRY;
            }
            message(DAWN);
        }
    }
}
const char *game_message(void) {
    static const char *messages[]={
        "COLETE E PREPARE SEU ABRIGO!", "+3 MADEIRA", "+2 PEDRA", "+2 COMIDA", "CONSTRUIDO! A DESMONTA.",
        "FALTAM RECURSOS!", "LOCAL OCUPADO!", "OLHE PARA UM RECURSO + A", "COMEU: +30 FOME, +8 VIDA",
        "SEM COMIDA. BUSQUE FRUTAS!", "FRIO! BUSQUE FOGO OU ABRIGO", "LOBO! A ATACA. BUSQUE FOGO", "ANOITECEU! FIQUE AQUECIDO.",
        "AMANHECEU! BUSQUE RECURSOS.", "DESMONTADO: METADE DE VOLTA", "LOBO AFASTADO! +1 COMIDA"
    };
    return messages[game.message];
}
