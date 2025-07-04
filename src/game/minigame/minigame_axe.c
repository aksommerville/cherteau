#include "game/game.h"

struct minigame_axe {
  struct minigame hdr;
};

#define MINIGAME ((struct minigame_axe*)minigame)

/* Delete.
 */
 
static void _axe_del(struct minigame *minigame) {
  free(minigame);
}

/* Update.
 */
 
static void _axe_update(struct minigame *minigame,double elapsed,int input,int pvinput) {
  //XXX L1 to lose, R1 to win
  if (!minigame->outcome) {
    if ((input&EGG_BTN_L1)&&!(pvinput&EGG_BTN_L1)) minigame->outcome=-1;
    if ((input&EGG_BTN_R1)&&!(pvinput&EGG_BTN_R1)) minigame->outcome=1;
  }
}

/* Render.
 */
 
static void _axe_render(struct minigame *minigame) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0xc0c0c0ff);//TODO
}

/* New.
 */
 
struct minigame *minigame_new_axe(double difficulty) {
  struct minigame *minigame=calloc(1,sizeof(struct minigame_axe));
  if (!minigame) return 0;
  minigame->name="axe";
  minigame->del=_axe_del;
  minigame->update=_axe_update;
  minigame->render=_axe_render;
  minigame->difficulty=(difficulty<=0.0)?0.0:(difficulty>=1.0)?1.0:difficulty;
  
  //TODO
  
  return minigame;
}
