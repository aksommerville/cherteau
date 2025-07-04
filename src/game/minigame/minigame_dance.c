#include "game/game.h"

struct minigame_dance {
  struct minigame hdr;
};

#define MINIGAME ((struct minigame_dance*)minigame)

/* Delete.
 */
 
static void _dance_del(struct minigame *minigame) {
  free(minigame);
}

/* Update.
 */
 
static void _dance_update(struct minigame *minigame,double elapsed,int input,int pvinput) {
  //XXX L1 to lose, R1 to win
  if (!minigame->outcome) {
    if ((input&EGG_BTN_L1)&&!(pvinput&EGG_BTN_L1)) minigame->outcome=-1;
    if ((input&EGG_BTN_R1)&&!(pvinput&EGG_BTN_R1)) minigame->outcome=1;
  }
}

/* Render.
 */
 
static void _dance_render(struct minigame *minigame) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0xc0c0c0ff);//TODO
}

/* New.
 */
 
struct minigame *minigame_new_dance(double difficulty) {
  struct minigame *minigame=calloc(1,sizeof(struct minigame_dance));
  if (!minigame) return 0;
  minigame->name="dance";
  minigame->del=_dance_del;
  minigame->update=_dance_update;
  minigame->render=_dance_render;
  minigame->difficulty=(difficulty<=0.0)?0.0:(difficulty>=1.0)?1.0:difficulty;
  
  //TODO
  
  return minigame;
}
