#include "game.h"
#define REG16(addr) (*(volatile u16 *)(addr))
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
static volatile u16 *screen=(volatile u16*)0x0600a000;
static int page=1;
static u32 uiTicks;
/* 0-19: environment (dimmed at night); 20-31: readable interface. */
static const u16 colors[32]={
    RGB(3,5,8),RGB(5,9,11),RGB(10,17,11),RGB(12,19,12),
    RGB(4,10,9),RGB(5,14,10),RGB(9,19,12),RGB(20,14,8),
    RGB(13,9,7),RGB(15,18,18),RGB(22,24,21),RGB(23,8,10),
    RGB(6,13,20),RGB(9,18,23),RGB(23,18,11),RGB(17,12,8),
    RGB(6,9,10),RGB(11,16,17),RGB(22,27,21),RGB(29,23,16),
    RGB(3,6,9),RGB(7,11,14),RGB(28,28,23),RGB(17,21,20),
    RGB(30,22,10),RGB(29,13,6),RGB(14,24,16),RGB(29,11,12),
    RGB(11,23,25),RGB(31,28,17),RGB(21,16,27),RGB(31,31,29)
};
/* Hand-authored 5x7 uppercase glyphs. Row bits run left to right. */
static const u8 font[36][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14}
};
static void rect(int x,int y,int w,int h,int c) {
    if(x<0) { w+=x; x=0; } if(y<0) { h+=y; y=0; }
    if(x+w>240) w=240-x;
    if(y+h>160) h=160-y;
    if(w<=0 || h<=0) return;
    u16 pair=(u16)(c|(c<<8));
    for(int row=y;row<y+h;row++) {
        volatile u16 *p=screen+row*120+x/2;
        int left=w;
        if(x&1) { *p=(u16)((*p&255)|(c<<8)); p++; left--; }
        while(left>=2) { *p++=pair; left-=2; }
        if(left) *p=(u16)((*p&0xff00)|c);
    }
}
static void border(int x,int y,int w,int h,int c) {
    rect(x,y,w,1,c); rect(x,y+h-1,w,1,c); rect(x,y,1,h,c); rect(x+w-1,y,1,h,c);
}
static void text(int x,int y,const char *s,int c,int scale) {
    while(*s) {
        char ch=*s++;
        const u8 *glyph=0;
        if(ch>='A' && ch<='Z') glyph=font[ch-'A'];
        if(ch>='0' && ch<='9') glyph=font[ch-'0'+26];
        for(int row=0;row<7;row++) {
            int bits=glyph ? glyph[row] : 0;
            if(ch==':' && (row==2 || row==5)) bits=4;
            if(ch=='.' && row==6) bits=4;
            if(ch=='-' && row==3) bits=14;
            if(ch=='/' && row<5) bits=1<<row;
            if(ch=='+' && row>=1 && row<=5) bits=row==3?31:4;
            if(ch=='!') bits=row<5 || row==6 ? 4:0;
            if(ch=='>' && row>=1 && row<=5) bits=1<<(row>3?row-1:5-row);
            for(int col=0;col<5;col++) if(bits&(16>>col)) rect(x+col*scale,y+row*scale,scale,scale,c);
        }
        x+=6*scale;
    }
}
static void centered(int y,const char *s,int c,int scale) {
    int n=0; while(s[n]) n++;
    text((240-(n*6-1)*scale)/2,y,s,c,scale);
}
static void number(int x,int y,int value,int c) {
    char digits[4]={'0','0',0,0};
    if(value<0) value=0;
    if(value>99) value=99;
    digits[0]=(char)('0'+value/10); digits[1]=(char)('0'+value%10);
    text(x,y,digits,c,1);
}
static void palette(void) {
    volatile u16 *pal=(volatile u16*)0x05000000;
    int dark=game.night && (game.state==PLAY || game.state==PAUSE || game.state==LOST);
    for(int i=0;i<32;i++) {
        u16 c=colors[i];
        if(dark && i<20) c=(u16)RGB((c&31)*3/5,((c>>5)&31)*3/5,((c>>10)&31)*3/4);
        pal[i]=c;
    }
}
static void tree(int x,int y) {
    rect(x+3,y+9,7,2,4); rect(x+5,y+7,2,4,7);
    rect(x+3,y,6,2,5); rect(x+2,y+2,8,4,5); rect(x+1,y+5,10,4,5);
    rect(x+4,y+1,3,2,6); rect(x+3,y+4,3,2,6); rect(x+2,y+7,4,1,6);
}
static void flame(int x,int y) {
    rect(x+1,y+9,10,2,9); rect(x+3,y+8,6,2,8);
    rect(x+3,y+4,6,5,25); rect(x+4,y+2,4,5,25);
    rect(x+5+(int)(uiTicks/8%2),y+1,2,4,24);
    rect(x+4,y+5,4,4,24); rect(x+5,y+6,2,3,29);
}
static void tile(int x,int y,int t,int gx,int gy) {
    int grass=((gx*3+gy)%7==0)?3:2;
    rect(x,y,12,12,t==WATER?12:grass);
    if(t==GRASS) {
        if((gx+gy*2)%5==0) { rect(x+3,y+6,1,2,6); rect(x+5,y+7,1,2,6); }
        if((gx*7+gy)%17==0) rect(x+8,y+3,1,1,14);
    } else if(t==WATER) {
        if((gx+gy)%3==0) rect(x+2+(int)(uiTicks/20%2),y+5,6,1,13);
    } else if(t==TREE) tree(x,y);
    else if(t==ROCK) {
        rect(x+2,y+8,9,3,4); rect(x+2,y+4,8,5,9); rect(x+4,y+2,5,6,9);
        rect(x+4,y+2,4,2,10); rect(x+3,y+4,2,3,10);
    } else if(t==BERRY) {
        rect(x+2,y+5,9,5,5); rect(x+4,y+3,5,6,6);
        rect(x+3,y+6,2,2,11); rect(x+7,y+4,2,2,11); rect(x+8,y+8,2,2,11);
    } else if(t==WALL) {
        rect(x,y+2,12,9,8); rect(x+1,y+1,3,9,7); rect(x+5,y+1,3,9,14);
        rect(x+9,y+1,2,9,7); rect(x,y+4,12,1,15); rect(x,y+8,12,1,15);
    } else if(t==FIRE) { rect(x+1,y+1,10,10,15); flame(x,y); }
    else if(t==SHELTER) {
        rect(x+1,y+4,10,8,7); rect(x+4,y+7,4,5,8);
        for(int i=0;i<5;i++) rect(x+5-i,y+i,2+i*2,1,14);
        rect(x+1,y+5,10,1,15); rect(x+2,y+7,1,4,14);
    }
}
static void player(int x,int y) {
    if(game.hitTimer && (game.hitTimer/3)%2) return;
    rect(x+3,y+10,7,2,4); rect(x+3,y+8,2,3,16); rect(x+7,y+8,2,3,16);
    rect(x+3,y+5,6,4,28); rect(x+2,y+6,2,3,19);
    rect(x+4,y+2,5,4,19); rect(x+3,y+1,6,2,8); rect(x+3,y,5,2,24);
    rect(x+2,y+2,8,1,24); rect(x+(game.dx<0?4:8),y+3,1,1,16);
    rect(x+9,y+6,1,4,7); rect(x+8,y+5,3,2,10);
}
static void wolf(int x,int y) {
    rect(x+2,y+5,7,4,17); rect(x+7,y+3,4,5,9); rect(x+7,y+2,1,2,16);
    rect(x+10,y+2,1,2,16); rect(x+9,y+5,1,1,27);
    rect(x+2,y+9,2,2,16); rect(x+7,y+9,2,2,16); rect(x,y+4,3,2,17);
}
static void world(void) {
    int cx=game.x-10,cy=game.y-4;
    if(cx<0) cx=0;
    if(cy<0) cy=0;
    if(cx>WORLD_W-20) cx=WORLD_W-20;
    if(cy>WORLD_H-9) cy=WORLD_H-9;
    for(int y=0;y<9;y++) for(int x=0;x<20;x++) tile(x*12,24+y*12,game_tile(cx+x,cy+y),cx+x,cy+y);
    int tx=(game.x+game.dx-cx)*12,ty=24+(game.y+game.dy-cy)*12;
    if(tx>=0 && tx<240 && ty>=24 && ty<132) border(tx,ty,12,12,game_tile(game.x+game.dx,game.y+game.dy)==GRASS?24:22);
    for(int i=0;i<WOLVES;i++) {
        Wolf *w=&game.wolves[i];
        int x=(w->x-cx)*12,y=24+(w->y-cy)*12;
        if(w->hp && x>=0 && x<240 && y>=24 && y<132) wolf(x,y);
    }
    player((game.x-cx)*12,24+(game.y-cy)*12);
    rect(0,0,240,24,20); rect(0,23,240,1,23);
    text(4,3,"V",27,1); rect(12,3,42,7,21); rect(13,4,game.health*40/100,5,27);
    text(60,3,"F",24,1); rect(68,3,42,7,21); rect(69,4,game.hunger*40/100,5,24);
    text(118,3,game.night?"NOITE":"DIA",game.night?30:24,1);
    number(154,3,game.nights+1,22);
    int seconds=((game.night?NIGHT_TICKS:DAY_TICKS)-game.phase+29)/30;
    number(184,3,seconds,22); text(197,3,"S",23,1);
    text(214,3,game_warm(game.x,game.y)?"OK":"--",26,1);
    text(4,14,"M:",14,1); number(16,14,game.wood,22);
    text(38,14,"P:",10,1); number(50,14,game.stone,22);
    text(72,14,"C:",27,1); number(84,14,game.food,22);
    text(118,14,"NOITES",23,1); number(160,14,game.nights,26); text(172,14,"/3",23,1);
    rect(202,14,32,6,21); rect(203,15,game.phase*30/(game.night?NIGHT_TICKS:DAY_TICKS),4,game.night?30:24);
    rect(0,132,240,28,20); rect(0,132,240,1,23);
    static const char *recipes[]={"B PAREDE  2M", "B FOGUEIRA 4M 2P", "B ABRIGO  6M 2P"};
    text(5,136,recipes[game.recipe],24,1); text(211,136,"L/R",23,1);
    text(5,149,game.messageTimer?game_message():"A COLETA  SELECT COME  START PAUSA",22,1);
}
static void title(void) {
    rect(0,0,240,160,20);
    for(int i=0;i<36;i++) rect((i*67+9)%240,(i*19+7)%91,1,1,i%3?23:24);
    rect(185,14,13,13,29); rect(181,11,12,12,20);
    centered(9,"UM PEQUENO MUNDO PARA SOBREVIVER",23,1);
    centered(30,"ILHA DO",24,2); centered(50,"ABRIGO",22,3);
    rect(14,111,212,12,12); rect(34,105,172,15,13);
    rect(49,101,143,18,14); rect(55,96,130,20,2); rect(69,90,103,23,3);
    tree(65,85); tree(79,80); tree(153,82); tree(170,89);
    tile(104,92,SHELTER,0,0); flame(131,96); player(119,94);
    rect(30,114,12,1,13); rect(198,116,17,1,13);
    centered(128,"START JOGAR",(uiTicks/22)%2?29:22,1);
    centered(141,"SELECT COMO JOGAR",23,1);
    centered(153,"SOBREVIVA A 3 NOITES",26,1);
}
static void instructions(int paused) {
    rect(7,7,226,147,20); border(7,7,226,147,24);
    centered(15,paused?"PAUSA":"COMO JOGAR",24,2);
    text(16,37,"DIRECIONAL  ANDAR / MIRAR",22,1);
    text(16,49,"A  COLETAR / ATACAR / DESMONTAR",22,1);
    text(16,61,"B  CONSTRUIR NO QUADRO A FRENTE",22,1);
    text(16,73,"L/R  ESCOLHER CONSTRUCAO",22,1);
    text(16,85,"SELECT  COMER      START  PAUSA",22,1);
    text(16,101,"FOGO: AQUECE E AFASTA LOBOS.",26,1);
    text(16,112,"ABRIGO: ENTRE PARA SE PROTEGER.",26,1);
    text(16,123,"F: SACIEDADE. NAO DEIXE ZERAR!",24,1);
    centered(140,paused?"START VOLTAR   B TITULO":"START JOGAR   B VOLTAR",23,1);
}
static void ending(void) {
    int won=game.state==WON;
    rect(15,32,210,97,20); border(15,32,210,97,won?26:27);
    centered(43,won?"VOCE VENCEU!":"FIM DE JOGO",won?26:27,2);
    centered(68,won?"TRES NOITES. UM NOVO LAR.":"A ILHA NAO PERDOA O DESPREPARO.",22,1);
    centered(83,won?"SEU ABRIGO RESISTIU.":"JUNTE COMIDA E ACENDA O FOGO.",23,1);
    centered(105,"START JOGAR DE NOVO",24,1);
    centered(117,"B TITULO",23,1);
}
static void wait_vblank(void) {
    while(REG16(0x04000006)>=160) {}
    while(REG16(0x04000006)<160) {}
}
static void sound(int bad) {
    REG16(0x04000068)=0x8180;
    REG16(0x0400006c)=(u16)(0xc000|(bad?1100:1750));
}
int main(void) {
    REG16(0x04000000)=0x0484; /* Force blank while preparing mode 4. */
    REG16(0x04000084)=0x0080;
    REG16(0x04000080)=0x2277;
    REG16(0x04000082)=0x0002;
    game.state=TITLE;
    /* Timer 2 runs at 16384 Hz. Fixed simulation steps are independent of
     * rendering cost: 4389/8 timer counts equals exactly two GBA frames. */
    REG16(0x04000108)=0;
    REG16(0x0400010a)=0x0083;
    u16 lastClock=REG16(0x04000108);
    unsigned previous=0,pending=0,accumulator=0;
    for(;;) {
        u16 clock=REG16(0x04000108);
        unsigned delta=(u16)(clock-lastClock); lastClock=clock;
        unsigned held=(~REG16(0x04000130))&1023;
        unsigned pressed=held&~previous; previous=held; uiTicks++;
        if(game.state==PLAY) { accumulator+=delta*8; pending|=pressed; }
        else { accumulator=0; pending=0; }
        int before=game.messageTimer;
        if(game.state==TITLE) {
            if(pressed&START) game_new(uiTicks*7919u);
            else if(pressed&SELECT) game.state=HELP;
        } else if(game.state==HELP) {
            if(pressed&(START|A)) game_new(uiTicks*7919u);
            else if(pressed&B) game.state=TITLE;
        } else if(game.state==PAUSE) {
            if(pressed&START) game.state=PLAY;
            else if(pressed&B) game.state=TITLE;
        } else if(game.state==WON || game.state==LOST) {
            if(pressed&START) game_new(uiTicks*7919u);
            else if(pressed&B) game.state=TITLE;
        } else {
            while(accumulator>=4389 && game.state==PLAY) {
                game_update(held,pending); pending=0; accumulator-=4389;
            }
        }
        if(game.messageTimer==90 && (before!=90 || (pressed&(A|B|SELECT))))
            sound(game.message==NEED_RESOURCES || game.message==COLD || game.message==ATTACKED);
        if(game.state==TITLE || game.state==HELP) {
            title(); if(game.state==HELP) instructions(0);
        } else {
            world();
            if(game.state==PAUSE) instructions(1);
            if(game.state==WON || game.state==LOST) ending();
        }
        wait_vblank();
        palette();
        REG16(0x04000000)=(u16)(0x0404|(page?0x10:0));
        page=!page; screen=(volatile u16*)(page?0x0600a000:0x06000000);
        /* Simulation time comes from Timer 2, not this render loop. */
    }
}
