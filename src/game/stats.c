#include "game.h"

/* We'll save the entire stats record as one string in Egg's store.
 * Strict COBOL-style formatting:
 *   MM:SS.mmm4321321321321321
 *   ---------____---___---___ len=25
 *         |    |  |  |  |  +-- danceoff
 *         |    |  |  |  +----- gold
 *         |    |  |  +-------- battlec
 *         |    |  +----------- winc
 *         |    +-------------- stepc
 *         +------------------- time
 * Everything's preformatted, but we'll turn the integer fields into integers.
 */

/* Load from store, or set defaults.
 */
 
#define DIGIT(ch) (((ch)>=0x30)&&((ch)<=0x39))
 
static void receive_time(char *dst,const char *src) {
  if (
    !DIGIT(src[0])||!DIGIT(src[1])||(src[2]!=':')||
    !DIGIT(src[3])||!DIGIT(src[4])||(src[5]!='.')||
    !DIGIT(src[6])||!DIGIT(src[7])||!DIGIT(src[8])
  ) {
    memcpy(dst,"99:99.999",9);
  } else {
    memcpy(dst,src,9);
  }
}

static void receive_int(int *dst,const char *src,int srcc,int dflt) {
  *dst=0;
  int i=srcc; while (i-->0) {
    if (!DIGIT(src[i])) {
      *dst=dflt;
      return;
    }
  }
  for (;srcc-->0;src++) {
    (*dst)*=10;
    (*dst)+=(*src)-'0';
  }
}
 
void stats_load(struct stats *stats) {
  char serial[25]={0};
  int serialc=egg_store_get(serial,sizeof(serial),"hiscore",7);
  if (serialc!=sizeof(serial)) memset(serial,0,sizeof(serial));
  receive_time(stats->time,serial);
  receive_int(&stats->stepc,serial+9,4,9999);
  receive_int(&stats->winc,serial+13,3,0);
  receive_int(&stats->battlec,serial+16,3,0);
  receive_int(&stats->gold,serial+19,3,0);
  receive_int(&stats->danceoff,serial+22,3,0);
}

/* Save to store.
 */
 
static void encode_int(char *dst,int dstc,int src) {
  if (src<0) src=0;
  int limit=10;
  int i=dstc; while (i-->0) limit*=10;
  if (src>=limit) src=limit-1;
  for (i=dstc;i-->0;src/=10) dst[i]='0'+src%10;
}
 
void stats_save(const struct stats *stats) {
  char serial[25];
  receive_time(serial,stats->time);
  encode_int(serial+9,4,stats->stepc);
  encode_int(serial+13,3,stats->winc);
  encode_int(serial+16,3,stats->battlec);
  encode_int(serial+19,3,stats->gold);
  encode_int(serial+22,3,stats->danceoff);
  egg_store_set("hiscore",7,serial,sizeof(serial));
}

/* Populate from session.
 */
 
static int limit_int(int src,int hi) {
  if (src<0) return 0;
  if (src>hi) return hi;
  return src;
}
 
void stats_from_game(struct stats *stats) {
  
  int ms=(int)(g.playtime*1000.0);
  if (ms<0) ms=0;
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  if (min>99) { min=sec=99; ms=999; }
  stats->time[0]='0'+min/10;
  stats->time[1]='0'+min%10;
  stats->time[2]=':';
  stats->time[3]='0'+sec/10;
  stats->time[4]='0'+sec%10;
  stats->time[5]='.';
  stats->time[6]='0'+ms/100;
  stats->time[7]='0'+(ms/10)%10;
  stats->time[8]='0'+ms%10;
  
  stats->stepc=limit_int(g.stepc,9999);
  stats->winc=limit_int(g.winc,999);
  stats->battlec=limit_int(g.battlec,999);
  stats->gold=limit_int(g.gold,999);
  stats->danceoff=limit_int(g.danceoff,999);
}
