#include "game/game.h"

struct minigame_karate {
  struct minigame hdr;
};

#define MINIGAME ((struct minigame_karate*)minigame)

/* Delete.
 */
 
static void _karate_del(struct minigame *minigame) {
  free(minigame);
}

/* Update.
 */
 
static void _karate_update(struct minigame *minigame,double elapsed,int input,int pvinput) {
  //XXX L1 to lose, R1 to win
  if (!minigame->outcome) {
    if ((input&EGG_BTN_L1)&&!(pvinput&EGG_BTN_L1)) minigame->outcome=-1;
    if ((input&EGG_BTN_R1)&&!(pvinput&EGG_BTN_R1)) minigame->outcome=1;
  }
}

/* Render.
 */
 
static void _karate_render(struct minigame *minigame) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0xc0c0c0ff);//TODO
}

/* New.
 */
 
struct minigame *minigame_new_karate(double difficulty) {
  struct minigame *minigame=calloc(1,sizeof(struct minigame_karate));
  if (!minigame) return 0;
  minigame->name="karate";
  minigame->del=_karate_del;
  minigame->update=_karate_update;
  minigame->render=_karate_render;
  minigame->difficulty=(difficulty<=0.0)?0.0:(difficulty>=1.0)?1.0:difficulty;
  
  //TODO
  
  return minigame;
}
