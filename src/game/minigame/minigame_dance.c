#include "game/game.h"

#define VIEW_TIME 2500 /* Width of framebuffer in song milliseconds. */
#define PAST_WIDTH 40 /* Horizontal position of "now". */
#define STRUCK_LEN 6 /* Track so many struck notes. We only need to remember them until they fall off the left edge. */
#define STRIKE_RADIUS 100 /* How far away from a note can we strike it, in ms. (one point) */
#define TWO_RADIUS 50 /* '' for two points */
#define THREE_RADIUS 20 /* '' for three points */
#define BIRD_RADIUS 80 /* Bird dances exactly to the music (and it's entirely decorative). Holds each frame for so long around each cue. */
#define COMPLAIN_TIME 0.300
#define CHEER_SPEED 30.0 /* px/s */
#define CHEER_LIMIT 16
#define REQ_LO 40 /* You have to really botch it to score this low. */
#define REQ_HI 75 /* I consistently score right around 90. But I know lots of people have trouble with rhythm games, we're not all musicians. 75 is laughably easy. */

static const struct cue {
  int ms;
  int btnid;
} cuev[]={
  2000,EGG_BTN_RIGHT,
  2750,EGG_BTN_DOWN,
  3000,EGG_BTN_DOWN,
  3750,EGG_BTN_DOWN,
  4000,EGG_BTN_RIGHT,
  4250,EGG_BTN_LEFT,
  4500,EGG_BTN_RIGHT,
  4750,EGG_BTN_LEFT,
  5000,EGG_BTN_DOWN,
  5750,EGG_BTN_DOWN,
  6000,EGG_BTN_LEFT,
  6250,EGG_BTN_RIGHT,
  6500,EGG_BTN_LEFT,
  6750,EGG_BTN_RIGHT,
  7000,EGG_BTN_UP,
  7500,EGG_BTN_RIGHT,
  8000,EGG_BTN_LEFT,
  8750,EGG_BTN_DOWN,
  9000,EGG_BTN_DOWN,
  9750,EGG_BTN_DOWN,
  10000,EGG_BTN_RIGHT,
  10250,EGG_BTN_LEFT,
  10500,EGG_BTN_RIGHT,
  10750,EGG_BTN_LEFT,
  11000,EGG_BTN_UP,
  11500,EGG_BTN_LEFT,
  12000,EGG_BTN_LEFT,
  12250,EGG_BTN_RIGHT,
  12500,EGG_BTN_DOWN,
  13750,EGG_BTN_DOWN,
  14000,EGG_BTN_RIGHT,
  14250,EGG_BTN_LEFT,
  14500,EGG_BTN_RIGHT,
  14750,EGG_BTN_LEFT,
  15000,EGG_BTN_UP,
  15500,EGG_BTN_DOWN,
  16000,EGG_BTN_UP,
};

struct minigame_dance {
  struct minigame hdr;
  int started;
  int cuep;
  int texid;
  int struckv[STRUCK_LEN]; // timestamp of notes recently struck, circular
  int struckp;
  int birdframe; // 0,1,2,3,4 = idle,down,right,left,up
  double complain[4]; // [down,right,left,up]. Countdown, while >0 show a complaint on that note.
  int score;
  int reqscore;
  struct cheer {
    double ttl;
    int x;
    double y;
    uint8_t tileid;
  } cheerv[CHEER_LIMIT];
  int cheerc;
  int singlec,doublec,triplec,missc;
};

#define MINIGAME ((struct minigame_dance*)minigame)

/* Delete.
 */
 
static void _dance_del(struct minigame *minigame) {
  egg_play_song(RID_song_lock_me_away,0,1);
  egg_texture_del(MINIGAME->texid);
  free(minigame);
}

/* Song over.
 */
 
static void dance_finish(struct minigame *minigame) {
  if (MINIGAME->score>=MINIGAME->reqscore) minigame->outcome=1;
  else minigame->outcome=-1;
  //fprintf(stderr,"Final score %d, single=%d, double=%d, triple=%d, miss=%d\n",MINIGAME->score,MINIGAME->singlec,MINIGAME->doublec,MINIGAME->triplec,MINIGAME->missc);
}

/* Add a cheer sprite.
 */
 
static void cheer(struct minigame *minigame,uint8_t tileid) {
  struct cheer *cheer=MINIGAME->cheerv;
  if (MINIGAME->cheerc<CHEER_LIMIT) {
    cheer=MINIGAME->cheerv+MINIGAME->cheerc++;
  } else {
    struct cheer *q=MINIGAME->cheerv;
    int i=CHEER_LIMIT;
    for (;i-->0;q++) {
      if (q->ttl<=0.0) { cheer=q; break; }
      if (q->ttl<cheer->ttl) cheer=q;
    }
  }
  cheer->x=PAST_WIDTH;
  cheer->y=60.0;
  cheer->tileid=tileid;
  cheer->ttl=1.000;
}

/* Strike key.
 */
 
static void strike_key(struct minigame *minigame,int btnid,int now) {
  const struct cue *cue=cuev;
  int i=sizeof(cuev)/sizeof(cuev[0]);
  for (;i-->0;cue++) {
    if (cue->btnid!=btnid) continue;
    int dms=now-cue->ms;
    if (dms<-STRIKE_RADIUS) break;
    if (dms>STRIKE_RADIUS) continue;
    int already=0;
    const int *p=MINIGAME->struckv;
    int j=STRUCK_LEN;
    for (;j-->0;p++) if (*p==cue->ms) {
      already=1;
      break;
    }
    if (already) continue;
    MINIGAME->struckv[MINIGAME->struckp++]=cue->ms;
    if (MINIGAME->struckp>=STRUCK_LEN) MINIGAME->struckp=0;
    if ((dms>-THREE_RADIUS)&&(dms<THREE_RADIUS)) { MINIGAME->score+=3; cheer(minigame,0x0b); MINIGAME->triplec++; }
    else if ((dms>-TWO_RADIUS)&&(dms<TWO_RADIUS)) { MINIGAME->score+=2; cheer(minigame,0x0a); MINIGAME->doublec++; }
    else { MINIGAME->score+=1; cheer(minigame,0x09); MINIGAME->singlec++; }
    return;
  }
  switch (btnid) {
    case EGG_BTN_DOWN: MINIGAME->complain[0]=COMPLAIN_TIME; break;
    case EGG_BTN_RIGHT: MINIGAME->complain[1]=COMPLAIN_TIME; break;
    case EGG_BTN_LEFT: MINIGAME->complain[2]=COMPLAIN_TIME; break;
    case EGG_BTN_UP: MINIGAME->complain[3]=COMPLAIN_TIME; break;
  }
  if (MINIGAME->score>0) MINIGAME->score--;
  MINIGAME->missc++;
}

/* Update.
 */
 
static void _dance_update(struct minigame *minigame,double elapsed,int input,int pvinput) {

  struct cheer *cheer=MINIGAME->cheerv;
  int i=MINIGAME->cheerc;
  for (;i-->0;cheer++) {
    cheer->ttl-=elapsed;
    cheer->y-=CHEER_SPEED*elapsed;
  }
  while (MINIGAME->cheerc&&(MINIGAME->cheerv[MINIGAME->cheerc-1].ttl<=0.0)) MINIGAME->cheerc--;

  if (minigame->outcome) return;
  int now=(int)(egg_audio_get_playhead()*1000.0);
  if (!now&&MINIGAME->started) {
    dance_finish(minigame);
    return;
  }
  if (!MINIGAME->started) {
    MINIGAME->started=1;
    egg_play_song(RID_song_dancing_bird,1,0);
  }
  #define BTN(tag) if ((input&EGG_BTN_##tag)&&!(pvinput&EGG_BTN_##tag)) strike_key(minigame,EGG_BTN_##tag,now);
  BTN(DOWN)
  BTN(RIGHT)
  BTN(LEFT)
  BTN(UP)
  #undef BTN
  
  MINIGAME->birdframe=0;
  const struct cue *cue=cuev;
  i=sizeof(cuev)/sizeof(cuev[0]);
  for (;i-->0;cue++) {
    int d=cue->ms-now;
    if (d>BIRD_RADIUS) break;
    if (d<-BIRD_RADIUS) continue;
    switch (cue->btnid) {
      case EGG_BTN_DOWN: MINIGAME->birdframe=1; break;
      case EGG_BTN_RIGHT: MINIGAME->birdframe=2; break;
      case EGG_BTN_LEFT: MINIGAME->birdframe=3; break;
      case EGG_BTN_UP: MINIGAME->birdframe=4; break;
    }
    break;
  }
  
  MINIGAME->complain[0]-=elapsed;
  MINIGAME->complain[1]-=elapsed;
  MINIGAME->complain[2]-=elapsed;
  MINIGAME->complain[3]-=elapsed;
}

/* Render.
 */
 
static void _dance_render(struct minigame *minigame) {
  graf_draw_rect(&g.graf,0,0,FBW,69,0xc080a0ff);
  graf_draw_rect(&g.graf,0,69,FBW,FBH,0x000000ff);
  graf_draw_rect(&g.graf,0,70,FBW,80,0xc0c0c0ff);
  
  // Chart lines.
  graf_draw_rect(&g.graf,0,80,FBW,1,0x808080ff);
  graf_draw_rect(&g.graf,0,100,FBW,1,0x808080ff);
  graf_draw_rect(&g.graf,0,120,FBW,1,0x808080ff);
  graf_draw_rect(&g.graf,0,140,FBW,1,0x808080ff);
  
  // Notes.
  int now=(int)(egg_audio_get_playhead()*1000.0);
  if (now) {
    const struct cue *cue=cuev;
    int i=sizeof(cuev)/sizeof(cuev[0]);
    for (;i-->0;cue++) {
      int x=((cue->ms-now)*FBW)/VIEW_TIME+PAST_WIDTH;
      if (x<-NS_sys_tilesize) continue;
      if (x>FBW+NS_sys_tilesize) break;
      int y;
      uint8_t tileid;
      switch (cue->btnid) {
        case EGG_BTN_DOWN:  tileid=0x00; y=140; break;
        case EGG_BTN_RIGHT: tileid=0x01; y=120; break;
        case EGG_BTN_LEFT:  tileid=0x02; y=100; break;
        case EGG_BTN_UP:    tileid=0x03; y=80; break;
      }
      int j=STRUCK_LEN;
      const int *p=MINIGAME->struckv;
      for (;j-->0;p++) if (*p==cue->ms) { tileid+=4; break; }
      graf_draw_tile(&g.graf,MINIGAME->texid,x,y,tileid,0);
    }
  }
  
  // X on each line if a complaint has been lodged.
  if (MINIGAME->complain[0]>0.0) graf_draw_tile(&g.graf,MINIGAME->texid,NS_sys_tilesize,140,0x08,0);
  if (MINIGAME->complain[1]>0.0) graf_draw_tile(&g.graf,MINIGAME->texid,NS_sys_tilesize,120,0x08,0);
  if (MINIGAME->complain[2]>0.0) graf_draw_tile(&g.graf,MINIGAME->texid,NS_sys_tilesize,100,0x08,0);
  if (MINIGAME->complain[3]>0.0) graf_draw_tile(&g.graf,MINIGAME->texid,NS_sys_tilesize,80,0x08,0);
  
  // "Now" indicator.
  graf_draw_rect(&g.graf,PAST_WIDTH,70,1,80,0xff0000a0);
  
  // Dot standing on the chart, dancing.
  int w=45,h=58,frame=0; // frame: idle,down,right,left,up
       if (g.input&EGG_BTN_DOWN) frame=1;
  else if (g.input&EGG_BTN_RIGHT) frame=2;
  else if (g.input&EGG_BTN_LEFT) frame=3;
  else if (g.input&EGG_BTN_UP) frame=4;
  int srcx=1+(w+1)*frame;
  int srcy=33;
  int dstx=80;
  int dsty=69-h;
  graf_draw_decal(&g.graf,MINIGAME->texid,dstx,dsty,srcx,srcy,w,h,0);
  
  // Bird, like Dot, on the right side.
  w=46;
  h=67;
  srcx=1+(w+1)*MINIGAME->birdframe;
  srcy=92;
  dstx=200;
  dsty=69-h;
  graf_draw_decal(&g.graf,MINIGAME->texid,dstx,dsty,srcx,srcy,w,h,0);
  
  // Cheers.
  struct cheer *cheer=MINIGAME->cheerv;
  int i=MINIGAME->cheerc;
  for (;i-->0;cheer++) {
    if (cheer->ttl<=0.0) continue;
    graf_draw_tile(&g.graf,MINIGAME->texid,cheer->x,(int)cheer->y,cheer->tileid,0);
  }
  
  // Score.
  {
    dsty=12;
    struct egg_draw_tile vtxv[7];
    int vtxc=0;
    if (MINIGAME->score>=100) vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'0'+(MINIGAME->score/100)%10,0};
    if (MINIGAME->score>=10)  vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'0'+(MINIGAME->score/10)%10,0};
    vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'0'+MINIGAME->score%10,0};
    vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'/',0};
    if (MINIGAME->reqscore>=100) vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'0'+(MINIGAME->reqscore/100)%10,0};
    if (MINIGAME->reqscore>=10)  vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'0'+(MINIGAME->reqscore/10)%10,0};
    vtxv[vtxc++]=(struct egg_draw_tile){0,dsty,'0'+MINIGAME->reqscore%10,0};
    w=vtxc*8;
    dstx=(FBW>>1)-(w>>1)+4;
    struct egg_draw_tile *vtx=vtxv;
    for (i=vtxc;i-->0;vtx++,dstx+=8) vtx->dstx=dstx;
    graf_flush(&g.graf);
    egg_draw_tile(1,g.texid_tilefont,vtxv,vtxc);
  }
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
  
  if (egg_texture_load_image(MINIGAME->texid=egg_texture_new(),RID_image_dance)<0) {
    _dance_del(minigame);
    return 0;
  }
  
  MINIGAME->reqscore=(int)(REQ_LO*(1.0-difficulty)+REQ_HI*difficulty);
  
  _dance_update(minigame,0.016,0,0);
  
  return minigame;
}
