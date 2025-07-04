#include "game/game.h"

// List of all things.
#define THING_EGG 1
#define THING_BONE 2
#define THING_LOG 3
#define THING_GOLD 4

/* Threshold for maintaining your power level, in terms of s/stroke averaged over a few strokes.
 * Experimentally just now, I find that 200 ms is stupid easy, 150 ms doable, and 130 ms heroically fast.
 * That's by my standards, and I'm a clumsy old man. I bet some whippersnapper out there can consistently beat 100 ms.
 * We don't want the game super difficult, it's for a game jam, so our fastest threshold will be just a hair hotter than "doable".
 */
#define SPEED_REQ_LO 0.170
#define SPEED_REQ_HI 0.145
#define SPEED_AVG_LEN 7

// Your opponent's power rises at a perfectly uniform rate, kind of boring.
#define SENSEI_RATE 0.100

struct minigame_karate {
  struct minigame hdr;
  struct rect lbox,rbox;
  struct rect lmeter,rmeter;
  struct egg_draw_tile frame[70];
  double lpower,rpower;
  int texid;
  int lthing,rthing;
  int lbroke,rbroke;
  int ldone,rdone;
  double clock; // Counts up forever.
  double avgv[SPEED_AVG_LEN];
  int avgp;
  double speed_threshold;
  double speed_threshlo,speed_threshhi;
  double playclock; // Counts down.
};

#define MINIGAME ((struct minigame_karate*)minigame)

/* Delete.
 */
 
static void _karate_del(struct minigame *minigame) {
  egg_texture_del(MINIGAME->texid);
  free(minigame);
}

/* How much power to break a thing? <1
 */
 
static double power_for_thing(int thing) {
  switch (thing) {
    case THING_EGG: return 0.333;
    case THING_BONE: return 0.500;
    case THING_LOG: return 0.666;
    case THING_GOLD: return 0.900;
  }
  return 0.500;
}

/* Update.
 */
 
static void _karate_update(struct minigame *minigame,double elapsed,int input,int pvinput) {
  MINIGAME->clock+=elapsed;
  
  if (MINIGAME->playclock>0.0) {
    if ((MINIGAME->playclock-=elapsed)<=0.0) {
      MINIGAME->ldone=1;
      MINIGAME->rdone=1;
      if (MINIGAME->lpower>power_for_thing(MINIGAME->lthing)) {
        MINIGAME->lbroke=1;
        if (MINIGAME->rpower>power_for_thing(MINIGAME->rthing)) { // if they both break it, more power wins
          MINIGAME->rbroke=1;
          if (MINIGAME->lpower>=MINIGAME->rpower) {
            minigame->outcome=1;
          } else {
            minigame->outcome=-1;
          }
        } else {
          minigame->outcome=1;
        }
      } else { // you didn't break it, you lose, regardless of what the other guy does.
        if (MINIGAME->rpower>power_for_thing(MINIGAME->rthing)) MINIGAME->rbroke=1;
        minigame->outcome=-1;
      }
    }
  }
  
  /* If the left player isn't done yet, track taps of SOUTH.
   * The tap rate for scoring purposes is a moving average of a small count of taps.
   * Recalculate that average every frame until (ldone), and add current time to it.
   * 
   */
  if (!MINIGAME->ldone) {
    if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
      MINIGAME->avgv[MINIGAME->avgp++]=MINIGAME->clock;
      if (MINIGAME->avgp>=SPEED_AVG_LEN) MINIGAME->avgp=0;
    }
    double ago=MINIGAME->avgv[MINIGAME->avgp];
    double now=MINIGAME->clock;
    double taprate=(now-ago)/SPEED_AVG_LEN;
    double velocity;
    if (taprate>=MINIGAME->speed_threshold) {
      double peakvel=-1.500;
      if (taprate>MINIGAME->speed_threshhi) taprate=MINIGAME->speed_threshhi;
      velocity=((taprate-MINIGAME->speed_threshold)*peakvel)/(MINIGAME->speed_threshhi-MINIGAME->speed_threshold);
    } else {
      double peakvel=0.500;
      if (taprate<MINIGAME->speed_threshlo) taprate=MINIGAME->speed_threshlo;
      velocity=((taprate-MINIGAME->speed_threshold)*peakvel)/(MINIGAME->speed_threshlo-MINIGAME->speed_threshold);
    }
    MINIGAME->lpower+=elapsed*velocity;
    if (MINIGAME->lpower>1.0) MINIGAME->lpower=1.0;
    else if (MINIGAME->lpower<0.0) MINIGAME->lpower=0.0;
  }
  
  if (!MINIGAME->rdone) {
    MINIGAME->rpower+=elapsed*SENSEI_RATE;
    if (MINIGAME->rpower>1.0) MINIGAME->rpower=1.0;
  }
}

/* Color for meter.
 */
 
static const struct colorstop { double p,r,g,b; } colorstopv[]={
  { 0.000, 0.000,0.000,0.000},
  { 0.250, 0.888,0.444,0.200},
  { 0.500, 0.999,0.999,0.100},
  { 0.750, 0.999,0.000,0.000},
  { 1.000, 0.999,0.999,0.999},
};
 
static uint32_t meter_color(double v) {
  const int colorstopc=sizeof(colorstopv)/sizeof(colorstopv[0]);
  const struct colorstop *lo=colorstopv,*hi=0;
  const struct colorstop *q=colorstopv;
  int i=colorstopc;
  for (;i-->0;q++) {
    if (q->p>=v) {
      hi=q;
      break;
    }
    lo=q;
  }
  if (!hi) hi=lo;
  double hiw=1.0;
  if (hi->p>lo->p) hiw=(v-lo->p)/(hi->p-lo->p);
  double low=1.0-hiw;
  int r=(int)((lo->r*low+hi->r*hiw)*256.0); if (r<0) r=0; else if (r>0xff) r=0xff;
  int g=(int)((lo->g*low+hi->g*hiw)*256.0); if (g<0) g=0; else if (g>0xff) g=0xff;
  int b=(int)((lo->b*low+hi->b*hiw)*256.0); if (b<0) b=0; else if (b>0xff) b=0xff;
  return (r<<24)|(g<<16)|(b<<8)|0xff;
}

/* Draw one combatant.
 */

static void draw_karatian(struct minigame *minigame,const struct rect *r,int srcx,int srcy,int srcw,int srch,double power,int thing,int broke) {
  graf_draw_decal(&g.graf,MINIGAME->texid,r->x+(r->w>>1)-(srcw>>1),r->y+r->h-srch,srcx,srcy,srcw,srch,0);
  graf_draw_decal(&g.graf,MINIGAME->texid,r->x+6,r->y+r->h-30,56,98,14,30,0);
  graf_draw_decal(&g.graf,MINIGAME->texid,r->x+r->w-20,r->y+r->h-30,56,98,14,30,0);
  
  int tsrcx,tsrcy,tw,th,tyoff=0;
  switch (thing) {
    case THING_EGG: {
        tsrcx=56;
        tw=59;
        if (broke) {
          tsrcy=31;
          th=36;
          tyoff=13;
        } else {
          tsrcy=1;
          th=29;
          tyoff=6;
        }
      } break;
    case THING_BONE: {
        tsrcx=116;
        if (broke) {
          tsrcy=23;
          tw=71;
          th=32;
          tyoff=15;
        } else {
          tsrcy=1;
          tw=74;
          th=21;
        }
      } break;
    case THING_LOG: {
        if (broke) {
          tsrcx=101;
          tsrcy=82;
          tw=88;
          th=26;
          tyoff=2;
        } else {
          tsrcx=116;
          tsrcy=56;
          tw=73;
          th=25;
          tyoff=1;
        }
      } break;
    case THING_GOLD: {
        if (broke) {
          tsrcx=101;
          tsrcy=125;
          tw=88;
          th=15;
        } else {
          tsrcx=117;
          tsrcy=109;
          tw=72;
          th=15;
        }
      } break;
    default: return;
  }
  int tdstx=r->x+(r->w>>1)-(tw>>1);
  int tdsty=r->y+r->h-30-th+tyoff;
  graf_draw_decal(&g.graf,MINIGAME->texid,tdstx,tdsty,tsrcx,tsrcy,tw,th,0);
  
  int hsrcx,hsrcy,hw,hh;
  if (r==&MINIGAME->lbox) {
    hsrcx=56;
    hsrcy=68;
    hw=22;
    hh=10;
  } else {
    hsrcx=56;
    hsrcy=79;
    hw=22;
    hh=12;
  }
  int hdstx=r->x+(r->w>>1)-(hw>>1);
  double hbottom=tdsty-4;
  double htop=r->y+r->h*0.333;
  int hdsty=(int)(htop*power+hbottom*(1.0-power));
  graf_draw_decal(&g.graf,MINIGAME->texid,hdstx,hdsty,hsrcx,hsrcy,hw,hh,0);
}

/* Render.
 */
 
static void _karate_render(struct minigame *minigame) {

  // Outer background.
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x0b211bff);
  
  // Box backgrounds.
  #define RECT(tag,color) graf_draw_rect(&g.graf,MINIGAME->tag.x,MINIGAME->tag.y,MINIGAME->tag.w,MINIGAME->tag.h,color);
  RECT(lbox,0x1d5445ff)
  RECT(lmeter,0x000000ff)
  RECT(rmeter,0x000000ff)
  RECT(rbox,0x1d5445ff)
  #undef RECT
  
  // Meter highlight areas.
  int h=(int)(MINIGAME->lpower*MINIGAME->lmeter.h);
  graf_draw_rect(&g.graf,MINIGAME->lmeter.x,MINIGAME->lmeter.y+MINIGAME->lmeter.h-h,MINIGAME->lmeter.w,h,meter_color(MINIGAME->lpower));
  h=(int)(MINIGAME->rpower*MINIGAME->rmeter.h);
  graf_draw_rect(&g.graf,MINIGAME->rmeter.x,MINIGAME->rmeter.y+MINIGAME->rmeter.h-h,MINIGAME->rmeter.w,h,meter_color(MINIGAME->rpower));
  
  // Dot and Sensei. TODO facial expressions.
  draw_karatian(minigame,&MINIGAME->lbox,1,1,54,127,MINIGAME->ldone?0.0:MINIGAME->lpower,MINIGAME->lthing,MINIGAME->lbroke);
  draw_karatian(minigame,&MINIGAME->rbox,1,129,75,120,MINIGAME->rdone?0.0:MINIGAME->rpower,MINIGAME->rthing,MINIGAME->rbroke);
  
  // Frame.
  graf_flush(&g.graf);
  egg_draw_tile(1,g.texid_sprites,MINIGAME->frame,70);
  
  // Clock.
  int sec=(int)(MINIGAME->playclock+0.999);
  if (sec>9) sec=9; else if (sec<0) sec=0;
  graf_draw_tile(&g.graf,g.texid_sprites,FBW>>1,10,0x30+sec,0);
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
  
  if (egg_texture_load_image(MINIGAME->texid=egg_texture_new(),RID_image_karate)<0) {
    _karate_del(minigame);
    return 0;
  }
  
  /* Establish bounds for the four boxes.
   * Our boxes have bounds that tuck under the borders by a pixel or two.
   * They are spaced by tile widths.
   */
  MINIGAME->lbox.w=MINIGAME->rbox.w=NS_sys_tilesize*6-6;
  MINIGAME->lbox.h=MINIGAME->rbox.h=NS_sys_tilesize*9-6;
  MINIGAME->lmeter.w=MINIGAME->rmeter.w=NS_sys_tilesize-8;
  MINIGAME->lmeter.h=MINIGAME->rmeter.h=MINIGAME->lbox.h;
  int totalw=NS_sys_tilesize*14;
  MINIGAME->lbox.x=(FBW>>1)-(totalw>>1); // initially ignore the left padding
  MINIGAME->lmeter.x=MINIGAME->lbox.x+NS_sys_tilesize*6;
  MINIGAME->rmeter.x=MINIGAME->lmeter.x+NS_sys_tilesize;
  MINIGAME->rbox.x=MINIGAME->rmeter.x+NS_sys_tilesize;
  MINIGAME->lbox.x+=3;
  MINIGAME->lmeter.x+=4;
  MINIGAME->rmeter.x+=4;
  MINIGAME->rbox.x+=3;
  MINIGAME->lbox.y=MINIGAME->lmeter.y=MINIGAME->rmeter.y=MINIGAME->rbox.y=NS_sys_tilesize+3;
  
  /* The box frames are a single grid of tiles, and we can set it up just once.
   */
  struct egg_draw_tile *vtx=MINIGAME->frame;
  #define ADDVTX(col,row,tileid) *(vtx++)=(struct egg_draw_tile){(col)*NS_sys_tilesize+(NS_sys_tilesize>>1),(row)*NS_sys_tilesize+(NS_sys_tilesize>>1),tileid,0};
  ADDVTX( 3, 1,0x40)
  ADDVTX( 8, 1,0x42)
  ADDVTX( 9, 1,0x43)
  ADDVTX(10, 1,0x43)
  ADDVTX(11, 1,0x40)
  ADDVTX(16, 1,0x42)
  ADDVTX( 3, 9,0x60)
  ADDVTX( 8, 9,0x62)
  ADDVTX( 9, 9,0x63)
  ADDVTX(10, 9,0x63)
  ADDVTX(11, 9,0x60)
  ADDVTX(16, 9,0x62)
  int col;
  for (col=4;col<8;col++) { ADDVTX(col,1,0x41) ADDVTX(col,9,0x61) }
  for (col=12;col<16;col++) { ADDVTX(col,1,0x41) ADDVTX(col,9,0x61) }
  int row;
  for (row=2;row<9;row++) {
    ADDVTX( 3,row,0x50)
    ADDVTX( 8,row,0x52)
    ADDVTX( 9,row,0x53)
    ADDVTX(10,row,0x53)
    ADDVTX(11,row,0x50)
    ADDVTX(16,row,0x52)
  }
  #undef ADDVTX
  
  MINIGAME->lpower=0.000;
  MINIGAME->rpower=0.000;
  if (minigame->difficulty<0.250) {
    MINIGAME->lthing=THING_EGG;
  } else if (minigame->difficulty<0.500) {
    MINIGAME->lthing=THING_BONE;
  } else if (minigame->difficulty<0.750) {
    MINIGAME->lthing=THING_LOG;
  } else {
    MINIGAME->lthing=THING_GOLD;
  }
  MINIGAME->rthing=THING_LOG;
  
  MINIGAME->speed_threshold=SPEED_REQ_LO*(1.0-minigame->difficulty)+SPEED_REQ_HI*minigame->difficulty;
  MINIGAME->speed_threshhi=MINIGAME->speed_threshold*2.0;
  MINIGAME->speed_threshlo=MINIGAME->speed_threshold*0.5;
  
  // Poison the speed average buffer with times in the distant past, 10 seconds ago.
  double *p=MINIGAME->avgv;
  int i=SPEED_AVG_LEN;
  for (;i-->0;p++) *p=-10.0;
  
  MINIGAME->playclock=9.0;
  
  return minigame;
}
