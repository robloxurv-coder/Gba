#include <assert.h>
#include <stdio.h>
#include "game.h"
static void tick(unsigned key) { game_update(key,key); }
static void fresh(void) { game_new(12345); }
static void ahead(int tile) { game.map[game.y+game.dy][game.x+game.dx]=(u8)tile; }
int main(void) {
    fresh(); assert(game.health==100 && game.state==PLAY);
    assert(game_tile(-1,0)==WATER && game_tile(40,0)==WATER);
    ahead(TREE); tick(RIGHT); assert(game.x==20); tick(A); assert(game.wood==11 && game_tile(21,16)==GRASS);
    ahead(ROCK); tick(A); assert(game.stone==6);
    ahead(BERRY); tick(A); assert(game.food==5);
    fresh(); tick(B); assert(game_tile(21,16)==FIRE && game.wood==4 && game.stone==2);
    assert(game_warm(20,16)); tick(A); assert(game.wood==6 && game.stone==3);
    fresh(); tick(L); tick(B); assert(game_tile(21,16)==WALL && game.wood==6);
    tick(RIGHT); assert(game.x==20); tick(A); assert(game_tile(21,16)==GRASS);
    fresh(); tick(R); tick(B); assert(game_tile(21,16)==SHELTER); tick(RIGHT); assert(game.x==21 && game_warm(21,16));
    fresh(); game.wood=0; tick(B); assert(game.message==NEED_RESOURCES && game.stone==4);
    fresh(); ahead(WATER); tick(B); assert(game.message==BLOCKED && game.wood==8);
    fresh(); game.hunger=20; game.health=80; tick(SELECT); assert(game.hunger==50 && game.health==88 && game.food==2);
    game.food=0; tick(SELECT); assert(game.message==NO_FOOD);
    fresh(); tick(START); u32 time=game.ticks; tick(A); assert(game.state==PAUSE && game.ticks==time);
    fresh(); game.phase=DAY_TICKS-1; tick(0); assert(game.night && game.phase==0);
    fresh(); game.night=1; game.ticks=119; tick(0); assert(game.health==96);
    fresh(); game.night=1; game.ticks=119; ahead(FIRE); tick(0); assert(game.health==100);
    fresh(); game.health=3; game.hunger=0; game.ticks=59; tick(0); assert(game.state==LOST);
    fresh(); game.wolves[0]=(Wolf){21,16,1}; tick(A); assert(!game.wolves[0].hp && game.food==4);
    fresh(); game.night=1; game.ticks=23; game.wolves[0]=(Wolf){21,16,1}; tick(0); assert(game.health==92);
    fresh(); game.night=1; game.phase=NIGHT_TICKS-1; game.nights=2; tick(0); assert(game.state==WON && game.nights==3);
    /* Full-length, unaccelerated rules simulation: fire + eating is winnable. */
    fresh(); tick(B);
    while(game.state==PLAY) {
        unsigned key=game.hunger<50 && game.food?SELECT:0;
        tick(key);
    }
    assert(game.state==WON);
    /* Reproducible fuzz run: map edges, resource caps, action combinations. */
    unsigned rng=1;
    for(int run=0;run<32;run++) {
        game_new((unsigned)run+1);
        for(int i=0;i<12000;i++) {
            rng=rng*1664525u+1013904223u;
            unsigned key=(rng>>20)&(A|B|SELECT|UP|DOWN|LEFT|RIGHT|L|R);
            game_update(key,key);
            assert(game.x>=2 && game.x<WORLD_W-2 && game.y>=2 && game.y<WORLD_H-2);
            assert(game.wood>=0 && game.wood<=99 && game.stone>=0 && game.stone<=99);
            assert(game.food>=0 && game.food<=99 && game.hunger>=0 && game.hunger<=100);
        }
    }
    puts("PASS: gathering, crafting, collision, warmth, food, pause, combat, death, victory, full run and 384000 fuzz updates.");
    return 0;
}
