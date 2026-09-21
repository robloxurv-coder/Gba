#ifndef GAME_H
#define GAME_H
/* Portable rules: no hardware access, heap, floating point or external assets. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
enum { WORLD_W=40, WORLD_H=32, DAY_TICKS=1800, NIGHT_TICKS=900, WOLVES=4 };
enum Tile { GRASS, WATER, TREE, ROCK, BERRY, WALL, FIRE, SHELTER };
enum Key { A=1, B=2, SELECT=4, START=8, RIGHT=16, LEFT=32, UP=64, DOWN=128, R=256, L=512 };
enum State { TITLE, HELP, PLAY, PAUSE, WON, LOST };
enum Message { WELCOME, WOOD_GAIN, STONE_GAIN, FOOD_GAIN, BUILT, NEED_RESOURCES,
    BLOCKED, NOTHING, EATEN, NO_FOOD, COLD, ATTACKED, NIGHTFALL, DAWN, REMOVED, WOLF_HIT };
typedef struct { int x,y,hp; } Wolf;
typedef struct {
    u8 map[WORLD_H][WORLD_W];
    Wolf wolves[WOLVES];
    u32 rng, ticks;
    int x,y,dx,dy,health,hunger,wood,stone,food;
    int phase,night,nights,recipe,state,message,messageTimer,moveTimer,hitTimer;
} Game;
extern Game game;
void game_new(u32 seed);
void game_update(unsigned held, unsigned pressed);
int game_tile(int x,int y);
int game_warm(int x,int y);
const char *game_message(void);
#endif
