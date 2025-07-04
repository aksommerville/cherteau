/* modal_encounter.c
 * We select the minigame and manage the common cruft around it.
 */

#include "game/game.h"

#define PULSE_TIME 0.300
#define PULSE_COUNT 3
#define MSG_LIMIT 256
#define TYPEWRITER_PERIOD 0.060

/* Instance definition.
 */
 
struct modal_encounter {
  struct modal hdr;
  double startclock;
  int startpulse;
  struct minigame *minigame;
  int outcome;
  struct egg_draw_tile msg[MSG_LIMIT];
  int msgc,msgp;
  double msgclock;
  struct rect msgr;
  double coolclock; // Extra cool-down time between minigame end and reporting results. (Give them a sec to stop tapping keys, etc).
};

#define MODAL ((struct modal_encounter*)modal)

/* Delete.
 */
 
static void _encounter_del(struct modal *modal) {
  if (MODAL->minigame) {
    MODAL->minigame->del(MODAL->minigame);
    MODAL->minigame=0;
  }
}

/* Init.
 */
 
static int _encounter_init(struct modal *modal) {
  MODAL->startclock=PULSE_TIME;
  MODAL->startpulse=PULSE_COUNT;
  modal->opaque=0;
  
  double difficulty=0.200;
  difficulty=(rand()&0xffff)/65535.0;
  const void *ctorv[]={
    minigame_new_karate,
    //minigame_new_dance,
    //minigame_new_axe,
    //minigame_new_jumprope,
  };
  int ctorc=sizeof(ctorv)/sizeof(ctorv[0]);
  int ctorp=rand()%ctorc;
  struct minigame *(*ctor)(double)=ctorv[ctorp];
  if (!(MODAL->minigame=ctor(difficulty))) return -1;
  
  return 0;
}

/* Prepare text message.
 */
 
static void encounter_set_message(struct modal *modal,int strix,int arg0) {
  struct strings_insertion insv[]={
    {'i',.i=arg0},
  };
  char tmp[MSG_LIMIT];
  int tmpc=strings_format(tmp,sizeof(tmp),1,strix,insv,sizeof(insv)/sizeof(insv[0]));
  if ((tmpc<0)||(tmpc>MSG_LIMIT)) tmpc=0;
  MODAL->msgc=break_text_tiles(MODAL->msg,MSG_LIMIT,&MODAL->msgr,tmp,tmpc);
  MODAL->msgp=0;
  MODAL->msgclock=0.0;
}

/* Minigame was won.
 */
 
static void encounter_win(struct modal *modal) {
  int prize=5;
  if ((g.gold+=prize)>999) g.gold=999;
  encounter_set_message(modal,3,prize);
}

/* Minigame was lost.
 */
 
static void encounter_lose(struct modal *modal) {
  if ((g.hp-=1)<=0) {
    g.hp=0;
    encounter_set_message(modal,5,0);
  } else {
    encounter_set_message(modal,4,0);
  }
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
  
  // Update the game, even if it's finished. But drop its input when finished.
  int input=g.input,pvinput=g.pvinput;
  if (MODAL->outcome) input=pvinput=0;
  MODAL->minigame->update(MODAL->minigame,elapsed,input,pvinput);
  
  // Detect establishment of outcome.
  if (MODAL->minigame->outcome&&!MODAL->outcome) {
    MODAL->coolclock=1.000;
    MODAL->outcome=MODAL->minigame->outcome;
    if (MODAL->outcome>0) {
      encounter_win(modal);
    } else {
      encounter_lose(modal);
    }
  }
  
  // Don't do any of the finishing stuff until coolclock chills fully.
  if (MODAL->coolclock>0.0) {
    MODAL->coolclock-=elapsed;
    return 1;
  }
  
  // When the outcome established and message fully typewritten, SOUTH or WEST dismisses us.
  if (MODAL->outcome&&(MODAL->msgp>=MODAL->msgc)) {
    if (
      ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH))||
      ((g.input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST))
    ) {
      return 0;
    }
  }
  
  // If outcome established, tick its typewriter clock.
  if (MODAL->outcome&&(MODAL->msgp<MODAL->msgc)) {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      egg_play_sound(RID_sound_typewriter_skip);
      MODAL->msgp=MODAL->msgc;
    } else {
      if ((MODAL->msgclock-=elapsed)<=0.0) {
        MODAL->msgclock+=TYPEWRITER_PERIOD;
        egg_play_sound(RID_sound_typewriter);
        MODAL->msgp++;
      }
    }
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
  
  MODAL->minigame->render(MODAL->minigame);
  
  /* If the outcome is established, draw the wrap-up message.
   */
  if (MODAL->outcome&&(MODAL->coolclock<=0.0)) {
    graf_draw_rect(&g.graf,MODAL->msgr.x,MODAL->msgr.y,MODAL->msgr.w,MODAL->msgr.h,0x000000ff);
    graf_flush(&g.graf);
    egg_draw_globals(0,0xff);
    egg_draw_tile(1,g.texid_tilefont,MODAL->msg,MODAL->msgp);
  }
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
