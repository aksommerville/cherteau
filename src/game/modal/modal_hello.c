/* modal_hello.c
 */
 
#include "game/game.h"

#define INIT_COOL_TIME 1.000
#define INIT_FADE_TIME 3.000

#define OPTION_LIMIT 3
#define STRIX_PLAY 21
#define STRIX_INPUT 22
#define STRIX_QUIT 23

struct modal_hello {
  struct modal hdr;
  int texid;
  double clock;
  struct option {
    int texid;
    int x,y,w,h;
    int strix;
  } optionv[OPTION_LIMIT];
  int optionc;
  int optionp;
};

#define MODAL ((struct modal_hello*)modal)

/* Cleanup.
 */
 
static void _hello_del(struct modal *modal) {
  egg_texture_del(MODAL->texid);
  struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    egg_texture_del(option->texid);
  }
}

/* Add option.
 */
 
static int add_option(struct modal *modal,int strix) {
  if (MODAL->optionc>=OPTION_LIMIT) return -1;
  struct option *option=MODAL->optionv+MODAL->optionc++;
  option->texid=font_texres_oneline(g.font,1,strix,FBW,0x80a0c0ff);
  egg_texture_get_status(&option->w,&option->h,option->texid);
  option->x=(FBW>>1)-(option->w>>1);
  option->strix=strix;
  return 0;
}

/* Init.
 */
 
static int _hello_init(struct modal *modal) {
  egg_play_song(0,0,0);//TODO new song
  
  if (egg_texture_load_image(MODAL->texid=egg_texture_new(),RID_image_hello)<0) return -1;
  
  add_option(modal,STRIX_PLAY);
  add_option(modal,STRIX_INPUT);
  add_option(modal,STRIX_QUIT);
  
  int y=120,spacing=15,i=MODAL->optionc;
  struct option *option=MODAL->optionv;
  for (;i-->0;option++,y+=spacing) {
    option->y=y;
  }
  
  return 0;
}

/* Update.
 */
 
static int _hello_update(struct modal *modal,double elapsed) {

  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    if (MODAL->clock<INIT_COOL_TIME+INIT_FADE_TIME) {
      MODAL->clock=INIT_COOL_TIME+INIT_FADE_TIME;
    } else if ((MODAL->optionp>=0)&&(MODAL->optionp<MODAL->optionc)) {
      const struct option *option=MODAL->optionv+MODAL->optionp;
      switch (option->strix) {
        case STRIX_PLAY: {
            world_reset();
            return 0;
          }
        case STRIX_INPUT: {
            egg_input_configure();
          } break;
        case STRIX_QUIT: {
            egg_terminate(0);
          } break;
      }
    }
  }
  if ((g.input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)&&(MODAL->optionc>1)) {
    egg_play_sound(RID_sound_uimotion);
    if (--(MODAL->optionp)<0) MODAL->optionp=MODAL->optionc-1;
  }
  if ((g.input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)&&(MODAL->optionc>1)) {
    egg_play_sound(RID_sound_uimotion);
    if (++(MODAL->optionp)>=MODAL->optionc) MODAL->optionp=0;
  }
  
  MODAL->clock+=elapsed;
  
  return 1;
}

/* Render.
 */
 
static void _hello_render(struct modal *modal) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  
  // "Cherteau" big and spooky.
  if (MODAL->clock<INIT_COOL_TIME) {
  } else {
    int srcx=1,srcy=1,w=231,h=72;
    int dstx=(FBW>>1)-(w>>1);
    int dsty=30;
    double t=MODAL->clock-INIT_COOL_TIME;
    if (t<INIT_FADE_TIME) {
      int alpha=(int)((t*256.0)/INIT_FADE_TIME);
      if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
    }
    graf_draw_decal(&g.graf,MODAL->texid,dstx,dsty,srcx,srcy,w,h,0);
    graf_set_alpha(&g.graf,0xff);
  }
  
  if (MODAL->clock>INIT_COOL_TIME+INIT_FADE_TIME) {
    struct option *option=MODAL->optionv;
    int i=0;
    for (;i<MODAL->optionc;i++,option++) {
      graf_draw_decal(&g.graf,option->texid,option->x,option->y,0,0,option->w,option->h,0);
      if (i==MODAL->optionp) {
        int w=22,h=11;
        graf_draw_decal(&g.graf,MODAL->texid,option->x-w-3,option->y+(option->h>>1)-(h>>1),233,1,w,h,0);
      }
    }
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_hello={
  .name="hello",
  .objlen=sizeof(struct modal_hello),
  .del=_hello_del,
  .init=_hello_init,
  .update=_hello_update,
  .render=_hello_render,
};
