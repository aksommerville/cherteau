/* modal_encounter.c
 * We select the minigame and manage the common cruft around it.
 */

#include "game/game.h"

#define PULSE_TIME 0.300
#define PULSE_COUNT 3

/* Instance definition.
 */
 
struct modal_encounter {
  struct modal hdr;
  double startclock;
  int startpulse;
};

#define MODAL ((struct modal_encounter*)modal)

/* Delete.
 */
 
static void _encounter_del(struct modal *modal) {
}

/* Init.
 */
 
static int _encounter_init(struct modal *modal) {
  MODAL->startclock=PULSE_TIME;
  MODAL->startpulse=PULSE_COUNT;
  modal->opaque=0;
  return 0;
}

/* Update.
 */
 
static int _encounter_update(struct modal *modal,double elapsed) {

  // Let startclock pay out before touching anything else.
  if (MODAL->startpulse>0) {
    if ((MODAL->startclock-=elapsed)<=0.0) {
      if (--(MODAL->startpulse)<=0) {
        modal->opaque=1;
      } else {
        MODAL->startclock+=PULSE_TIME;
      }
    }
    return 1;
  }
  
  //TODO
  if ((g.input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) {
    return 0;
  }
  return 1;
}

/* Render.
 */
 
static void _encounter_render(struct modal *modal) {

  /* When startclock is still counting down, we're transparent and we do kind of a blinking effect.
   */
  if (MODAL->startpulse>0) {
    double t=MODAL->startclock/PULSE_TIME;
    t-=(int)t;
    if (t>0.5) t=1.0-t;
    t*=2.0;
    int alpha=(int)(t*255.0);
    if (alpha>0) {
      if (alpha>0xff) alpha=0xff;
      graf_draw_rect(&g.graf,0,0,FBW,FBH,0xc0103000|alpha);
    }
    return;
  }
  
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
}

/* Type definition.
 */
 
const struct modal_type modal_type_encounter={
  .name="encounter",
  .objlen=sizeof(struct modal_encounter),
  .del=_encounter_del,
  .init=_encounter_init,
  .update=_encounter_update,
  .render=_encounter_render,
};
