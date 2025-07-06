/* modal_gameover.c
 */
 
#include "game/game.h"

// It's 385 as reported by the file, but its declared tempo appears to disagree with its event timing.
#define SONG_TEMPO_MS 389

#define MSGID_FIRST 16
#define MSGID_LAST 20
#define MSG_TIME 5.000 /* Including fades. */
#define MSG_FADE_IN_TIME 0.500
#define MSG_FADE_OUT_TIME 1.000

struct modal_gameover {
  struct modal hdr;
  int texid; // dance
  int texid_stats;
  int statsw,statsh;
  double clock; // Counts up forever.
  double bgr,bgg,bgb; // Background color.
  double bgrd,bggd,bgbd;
  int msgid; // Show stats if >MSGID_LAST
  int texid_msg;
  int msgw,msgh;
  double msgclock; // Counts up.
};

#define MODAL ((struct modal_gameover*)modal)

/* Cleanup.
 */
 
static void _gameover_del(struct modal *modal) {
  egg_texture_del(MODAL->texid);
  egg_texture_del(MODAL->texid_stats);
  egg_texture_del(MODAL->texid_msg);
}

/* Prepare stats report.
 * This will also trigger saving the new high scores.
 *             Now        Best
 *       Time: MM:SS.mmm *MM:SS.mmm
 *      Steps: N         *N
 *    Battles: WIN/PLAY  *WIN/PLAY
 *       Gold: N         *N
 *  Dance-off: N         *N
 */
 
static void render_int_field(void *image,int w,int h,int stride,int x,int y,int v,uint32_t color) {
  if (v<0) v=0; else if (v>999999) v=999999;
  char tmp[6];
  int tmpc=0;
  if (v>=100000) tmp[tmpc++]='0'+(v/1000000);
  if (v>=10000 ) tmp[tmpc++]='0'+(v/10000)%10;
  if (v>=1000  ) tmp[tmpc++]='0'+(v/1000)%10;
  if (v>=100   ) tmp[tmpc++]='0'+(v/100)%10;
  if (v>=10    ) tmp[tmpc++]='0'+(v/10)%10;
  tmp[tmpc++]='0'+v%10;
  font_render_string(image,w,h,stride,x,y,g.font,tmp,tmpc,color);
}
 
static void render_int2_field(void *image,int w,int h,int stride,int x,int y,int a,int b,uint32_t color) {
  if (a<0) a=0; else if (a>999) a=999; // Only thing we use this for is winc/battlec, which are both in 0..999
  if (b<0) b=0; else if (b>999) b=999;
  char tmp[7];
  int tmpc=0;
  if (a>=100) tmp[tmpc++]='0'+a/100;
  if (a>=10) tmp[tmpc++]='0'+(a/10)%10;
  tmp[tmpc++]='0'+a%10;
  tmp[tmpc++]='/';
  if (b>=100) tmp[tmpc++]='0'+b/100;
  if (b>=10) tmp[tmpc++]='0'+(b/10)%10;
  tmp[tmpc++]='0'+b%10;
  font_render_string(image,w,h,stride,x,y,g.font,tmp,tmpc,color);
}

static int gameover_prepare_report(struct modal *modal) {
  
  /* Gather stats and compare to the high scores.
   */
  struct stats stats,nbest=g.stats_best;
  struct stats highlight={0}; // Nonzero if it's a new record or tied.
  stats_from_game(&stats);
  if (memcmp(stats.time,g.stats_best.time,sizeof(stats.time))<=0) {
    highlight.time[0]=1;
    memcpy(nbest.time,stats.time,sizeof(stats.time));
  }
  #define LOWEST_INT(fld) if (stats.fld<=g.stats_best.fld) { \
    highlight.fld=1; \
    nbest.fld=stats.fld; \
  }
  #define HIGHEST_INT(fld) if (stats.fld>=g.stats_best.fld) { \
    highlight.fld=1; \
    nbest.fld=stats.fld; \
  }
  LOWEST_INT(stepc)
  HIGHEST_INT(winc)
  HIGHEST_INT(battlec)
  HIGHEST_INT(gold)
  HIGHEST_INT(danceoff)
  #undef LOWEST_INT
  #undef HIGHEST_INT
  
  /* If at least one field has a highlight, save it.
   * This means we're saving unnecessarily in tie cases. Whatever.
   */
  if (highlight.time[0]||highlight.stepc||highlight.winc||highlight.battlec||highlight.gold||highlight.danceoff) {
    stats_save(&nbest);
    g.stats_best=nbest;
  }
  
  /* Prepare the image.
   * Full width, and 6 rows.
   */
  int rowh=font_get_line_height(g.font)+1;
  int imgw=FBW;
  int imgh=rowh*6;
  int stride=FBW<<2;
  int colonx=130; // Left column right-aligns here. Middle column left-aligns a bit to the right of it.
  int col1x=colonx+5;
  int colw=60;
  uint32_t static_color=0x808080ff;
  uint32_t data_color=0xffffffff;
  uint32_t highlight_color=0xffff00ff;
  uint8_t *rgba=calloc(stride,imgh);
  if (!rgba) return -1;
  
  /* Static text labels.
   */
  const char *src;
  int srcc;
  if ((srcc=strings_get(&src,1,9))>0) font_render_string(rgba,imgw,imgh,stride,col1x,0,g.font,src,srcc,static_color); // "Now"
  if ((srcc=strings_get(&src,1,10))>0) font_render_string(rgba,imgw,imgh,stride,col1x+colw,0,g.font,src,srcc,static_color); // "Best"
  if ((srcc=strings_get(&src,1,11))>0) { // "Time"
    int srcw=font_measure_line(g.font,src,srcc);
    font_render_string(rgba,imgw,imgh,stride,colonx-srcw,rowh,g.font,src,srcc,static_color);
  }
  if ((srcc=strings_get(&src,1,12))>0) { // "Steps"
    int srcw=font_measure_line(g.font,src,srcc);
    font_render_string(rgba,imgw,imgh,stride,colonx-srcw,rowh*2,g.font,src,srcc,static_color);
  }
  if ((srcc=strings_get(&src,1,13))>0) { // "Battles"
    int srcw=font_measure_line(g.font,src,srcc);
    font_render_string(rgba,imgw,imgh,stride,colonx-srcw,rowh*3,g.font,src,srcc,static_color);
  }
  if ((srcc=strings_get(&src,1,14))>0) { // "Gold"
    int srcw=font_measure_line(g.font,src,srcc);
    font_render_string(rgba,imgw,imgh,stride,colonx-srcw,rowh*4,g.font,src,srcc,static_color);
  }
  if ((srcc=strings_get(&src,1,15))>0) { // "Dance-Off"
    int srcw=font_measure_line(g.font,src,srcc);
    font_render_string(rgba,imgw,imgh,stride,colonx-srcw,rowh*5,g.font,src,srcc,static_color);
  }
  
  /* Data.
   */
  if (highlight.time[0]) {
    font_render_string(rgba,imgw,imgh,stride,col1x,rowh,g.font,stats.time,sizeof(stats.time),highlight_color);
    font_render_string(rgba,imgw,imgh,stride,col1x+colw,rowh,g.font,g.stats_best.time,sizeof(stats.time),highlight_color);
  } else {
    font_render_string(rgba,imgw,imgh,stride,col1x,rowh,g.font,stats.time,sizeof(stats.time),data_color);
    font_render_string(rgba,imgw,imgh,stride,col1x+colw,rowh,g.font,g.stats_best.time,sizeof(stats.time),data_color);
  }
  render_int_field(rgba,imgw,imgh,stride,col1x,rowh*2,stats.stepc,highlight.stepc?highlight_color:data_color);
  render_int_field(rgba,imgw,imgh,stride,col1x+colw,rowh*2,g.stats_best.stepc,highlight.stepc?highlight_color:data_color);
  render_int2_field(rgba,imgw,imgh,stride,col1x,rowh*3,stats.winc,stats.battlec,highlight.winc?highlight_color:data_color);
  render_int2_field(rgba,imgw,imgh,stride,col1x+colw,rowh*3,g.stats_best.winc,g.stats_best.battlec,highlight.winc?highlight_color:data_color);
  render_int_field(rgba,imgw,imgh,stride,col1x,rowh*4,stats.gold,highlight.gold?highlight_color:data_color);
  render_int_field(rgba,imgw,imgh,stride,col1x+colw,rowh*4,g.stats_best.gold,highlight.gold?highlight_color:data_color);
  render_int_field(rgba,imgw,imgh,stride,col1x,rowh*5,stats.danceoff,highlight.danceoff?highlight_color:data_color);
  render_int_field(rgba,imgw,imgh,stride,col1x+colw,rowh*5,g.stats_best.danceoff,highlight.danceoff?highlight_color:data_color);
  
  /* Load to a texture, free the pixels, and we're done.
   */
  MODAL->texid_stats=egg_texture_new();
  egg_texture_load_raw(MODAL->texid_stats,imgw,imgh,stride,rgba,stride*imgh);
  free(rgba);
  egg_texture_get_status(&MODAL->statsw,&MODAL->statsh,MODAL->texid_stats);
  return 0;
}

/* Init.
 */
 
static int _gameover_init(struct modal *modal) {

  /* Play the battle song.
   * Maybe too exciting for the gameover music? Usually we do something soothing.
   * But I don't feel like writing another song, and this one's really good,
   * and the user has probly only heard the first 10 sec of it so far :P
   */
  egg_play_song(RID_song_even_tippier_toe,0,1);
  
  /* Load the dance-off graphics.
   */
  if (egg_texture_load_image(MODAL->texid=egg_texture_new(),RID_image_dance)<0) return -1;
  
  /* Collect stats for reporting.
   * We'll print these into a texture right now, the whole report.
   */
  if (gameover_prepare_report(modal)<0) return -1;
  
  /* Prepare stage background color animation.
   */
  MODAL->bgr=MODAL->bgg=MODAL->bgb=0.666;
  MODAL->bgrd=0.111;
  MODAL->bggd=0.300;
  MODAL->bgbd=0.077;
  
  return 0;
}

/* Start showing the message indicated by (msgid).
 * Safe to call even for OOB ids.
 */
 
static void prepare_msg(struct modal *modal) {
  MODAL->msgclock=0.0;
  if ((MODAL->msgid<MSGID_FIRST)||(MODAL->msgid>MSGID_LAST)) return;
  egg_texture_del(MODAL->texid_msg);
  MODAL->texid_msg=font_texres_multiline(g.font,1,MODAL->msgid,FBW,FBH,0xffffffff);
  egg_texture_get_status(&MODAL->msgw,&MODAL->msgh,MODAL->texid_msg);
}

/* Update.
 */
 
static int _gameover_update(struct modal *modal,double elapsed) {
  MODAL->clock+=elapsed;
  
  // Tapping A jumps to the stats, or if we're already there, terminates.
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    if (MODAL->msgid<=MSGID_LAST) {
      MODAL->msgid=MSGID_LAST+1;
      prepare_msg(modal);
    } else {
      return 0;
    }
  }

  const double bglo=0.200,bghi=0.950;
  #define BGANIM(ch) { \
    MODAL->bg##ch+=MODAL->bg##ch##d*elapsed; \
    if (MODAL->bg##ch<bglo) { MODAL->bg##ch=bglo; MODAL->bg##ch##d=-MODAL->bg##ch##d; } \
    else if (MODAL->bg##ch>bghi) { MODAL->bg##ch=bghi; MODAL->bg##ch##d=-MODAL->bg##ch##d; } \
  }
  BGANIM(r)
  BGANIM(g)
  BGANIM(b)
  #undef BGANIM
  
  // No message? Check the clock.
  if (!MODAL->msgid) {
    if (MODAL->clock<2.000) {
      // A little initial black time.
    } else {
      MODAL->msgid=MSGID_FIRST+(int)((MODAL->clock-2.000)/MSG_TIME);
      if (MODAL->msgid<MSGID_FIRST) MODAL->msgid=MSGID_FIRST;
      prepare_msg(modal);
    }
    
  // Message or stats showing. Tick the message clock.
  } else {
    MODAL->msgclock+=elapsed;
    // If it's a real message and the clock expires, load the next one.
    if ((MODAL->msgid<=MSGID_LAST)&&(MODAL->msgclock>=MSG_TIME)) {
      MODAL->msgid++;
      prepare_msg(modal);
    }
  }
  
  return 1;
}

/* Render the stage: Dot and the Bird dance over a colored strip.
 */
 
static void render_stage(struct modal *modal,int limx,int limy,int limw,int limh) {

  // Starts with nothing, then the stage vips on like a CRT.
  int x,y,w,h;
  if (MODAL->clock<0.500) { // A little breather time.
    return;
  } else if (MODAL->clock<1.000) { // Single row expanding horizontally.
    double t=(MODAL->clock-0.500)/0.500;
    w=(int)(t*limw);
    if (w<1) return;
    if (w>limw) w=limw;
    h=1;
    x=limx+(limw>>1)-(w>>1);
    y=limy+(limh>>1);
  } else if (MODAL->clock<1.500) { // Full width expanding vertically.
    double t=(MODAL->clock-1.000)/0.500;
    h=(int)(t*limh);
    if (h<1) h=1;
    else if (h>limh) h=limh;
    x=limx;
    w=limw;
    y=limy+(limh>>1)-(h>>1);
  } else { // The whole thing.
    x=limx;
    y=limy;
    w=limw;
    h=limh;
  }
  
  // Background color gets animated by update.
  int r=(int)(MODAL->bgr*255.0); if (r<0) r=0; else if (r>0xff) r=0xff;
  int G=(int)(MODAL->bgg*255.0); if (G<0) G=0; else if (G>0xff) G=0xff;
  int b=(int)(MODAL->bgb*255.0); if (b<0) b=0; else if (b>0xff) b=0xff;
  graf_draw_rect(&g.graf,x,y,w,h,(r<<24)|(G<<16)|(b<<8)|0xff);
  
  // Dot and the Bird do the traditional Game Over Dance.
  if (MODAL->clock>=2.000) {
    double ph=egg_audio_get_playhead();
    int phms=(int)(ph*1000.0);
    int songframe=phms/SONG_TEMPO_MS;
    int frame=0;
    switch (songframe%16) {
      case 1: frame=1; break;
      case 3: frame=2; break;
      case 5: frame=3; break;
      case 7: frame=2; break;
      case 9: frame=3; break;
      case 11: frame=2; break;
      case 13: frame=3; break;
      case 15: frame=4; break;
    }
    int dotsrcx=1,dotsrcy=33,dotw=45,doth=58;
    int birdsrcx=1,birdsrcy=92,birdw=46,birdh=67;
    int dotx=100,doty=y+h-doth;
    int birdx=180,birdy=y+h-birdh;
    graf_draw_decal(&g.graf,MODAL->texid,dotx,doty,dotsrcx+(dotw+1)*frame,dotsrcy,dotw,doth,0);
    graf_draw_decal(&g.graf,MODAL->texid,birdx,birdy,birdsrcx+(birdw+1)*frame,birdsrcy,birdw,birdh,0);
  }
}

/* Render.
 */
 
static void _gameover_render(struct modal *modal) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  
  /* Dot and the Bird dance in a narrow horizontal strip with floopulating background color.
   */
  int stagex=0,stagey=20,stagew=FBW,stageh=70;
  render_stage(modal,stagex,stagey,stagew,stageh);
  
  /* Message or stats.
   */
  if (MODAL->msgid>MSGID_LAST) {
    int statsy=((stagey+stageh+FBH)>>1)-(MODAL->statsh>>1);
    if (MODAL->msgclock<MSG_FADE_IN_TIME) {
      int alpha=(int)((MODAL->msgclock/MSG_FADE_IN_TIME)*255.0);
      if (alpha<=0) return;
      if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
    }
    graf_draw_decal(&g.graf,MODAL->texid_stats,(FBW>>1)-(MODAL->statsw>>1),statsy,0,0,MODAL->statsw,MODAL->statsh,0);
    graf_set_alpha(&g.graf,0xff);
  } else if (MODAL->msgid>=MSGID_FIRST) {
    int msgx=(FBW>>1)-(MODAL->msgw>>1);
    int msgy=((stagey+stageh+FBH)>>1)-(MODAL->msgh>>1);
    if (MODAL->msgclock<MSG_FADE_IN_TIME) {
      int alpha=(int)((MODAL->msgclock/MSG_FADE_IN_TIME)*255.0);
      if (alpha<=0) return;
      if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
    } else if (MODAL->msgclock>MSG_TIME-MSG_FADE_OUT_TIME) {
      int alpha=(int)(((MSG_TIME-MODAL->msgclock)/MSG_FADE_OUT_TIME)*255.0);
      if (alpha<=0) return;
      if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
    }
    graf_draw_decal(&g.graf,MODAL->texid_msg,msgx,msgy,0,0,MODAL->msgw,MODAL->msgh,0);
    graf_set_alpha(&g.graf,0xff);
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_gameover={
  .name="gameover",
  .objlen=sizeof(struct modal_gameover),
  .del=_gameover_del,
  .init=_gameover_init,
  .update=_gameover_update,
  .render=_gameover_render,
};
