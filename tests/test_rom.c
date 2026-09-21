/* Actual-ROM integration test. Requires libmgba-dev on the host, not on GBA.
 * Arguments: ROM path, hexadecimal address of 'game' from arm-none-eabi-nm.
 * All interactions use emulated buttons. Memory is only read for assertions. */
#include <mgba/core/core.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"
static struct mCore *core;
static color_t video[240*160];
static unsigned base;
static unsigned read_field(size_t offset) { return core->busRead32(core,base+(unsigned)offset); }
#define FIELD(name) read_field(offsetof(Game,name))
static void frames(int n,unsigned keys) {
    core->setKeys(core,keys);
    for(int i=0;i<n;i++) core->runFrame(core);
}
static void press(unsigned keys) { frames(4,keys); frames(10,0); }
static unsigned tile_at(int x,int y) { return core->busRead8(core,base+y*WORLD_W+x); }
static void screenshot(const char *path) {
    FILE *f=fopen(path,"wb"); assert(f);
    fprintf(f,"P6\n240 160\n255\n");
    for(int i=0;i<240*160;i++) {
        unsigned c=video[i];
        unsigned char rgb[]={c&255,(c>>8)&255,(c>>16)&255};
        fwrite(rgb,1,3,f);
    }
    fclose(f);
}
int main(int argc,char **argv) {
    assert(argc==3); base=(unsigned)strtoul(argv[2],NULL,16);
    core=mCoreFind(argv[1]); assert(core && core->init(core));
    mCoreInitConfig(core,NULL);
    core->opts.useBios=false; core->opts.skipBios=true;
    core->setVideoBuffer(core,video,240);
    assert(mCoreLoadFile(core,argv[1])); core->reset(core);
    frames(100,0); assert(FIELD(state)==TITLE);
    assert((core->busRead16(core,0x04000000)&0x487)==0x404);
    screenshot("build/title.ppm");
    press(SELECT); assert(FIELD(state)==HELP); screenshot("build/help.ppm");
    press(START); assert(FIELD(state)==PLAY && FIELD(x)==20);
    unsigned ticks=FIELD(ticks); frames(600,0);
    unsigned elapsed=FIELD(ticks)-ticks;
    printf("Timing: %u simulation updates / 600 hardware frames\n",elapsed);
    assert(elapsed>=295 && elapsed<=305);
    press(RIGHT); press(RIGHT); press(RIGHT); assert(FIELD(x)==23);
    press(A); assert(FIELD(wood)==11 && tile_at(24,16)==GRASS);
    press(B); assert(FIELD(wood)==7 && tile_at(24,16)==FIRE);
    press(DOWN); press(DOWN); assert(FIELD(y)==18);
    press(RIGHT); assert(FIELD(x)==23); press(A); assert(FIELD(stone)==4);
    press(L); press(B); assert(tile_at(24,18)==WALL);
    press(A); assert(FIELD(wood)==6 && tile_at(24,18)==GRASS);
    press(R); press(R); press(B); assert(tile_at(24,18)==SHELTER);
    press(RIGHT); assert(FIELD(x)==24);
    screenshot("build/gameplay.ppm");
    press(SELECT); assert(FIELD(food)==2);
    press(START); assert(FIELD(state)==PAUSE); screenshot("build/pause.ppm");
    ticks=FIELD(ticks); frames(180,0); assert(FIELD(ticks)==ticks);
    press(START); assert(FIELD(state)==PLAY);
    int nightShot=0,iterations=0;
    while(FIELD(state)==PLAY && iterations++<320) {
        if(FIELD(hunger)<50 && FIELD(food)) press(SELECT);
        frames(60,0);
        if(FIELD(night) && !nightShot) { screenshot("build/night.ppm"); nightShot=1; }
    }
    assert(FIELD(state)==WON && FIELD(nights)==3);
    frames(10,0); screenshot("build/victory.ppm");
    press(START); assert(FIELD(state)==PLAY && FIELD(nights)==0);
    iterations=0;
    while(FIELD(state)==PLAY && iterations++<320) frames(60,0);
    assert(FIELD(state)==LOST && FIELD(health)==0);
    frames(10,0); screenshot("build/game_over.ppm");
    press(B); assert(FIELD(state)==TITLE);
    puts("PASS: real ROM boot, help, frame timing, movement, gathering, all recipes, eating, pause, three nights, win, restart, death, title.");
    core->deinit(core); mCoreConfigDeinit(&core->config); free(core);
    return 0;
}
