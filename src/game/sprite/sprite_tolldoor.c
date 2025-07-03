/* sprite_tolldoor.c
 * arg: u8:tollid u24:reserved
 */

#include "game/game.h"

struct sprite_tolldoor {
  struct sprite hdr;
  uint8_t tollid;
};

#define SPRITE ((struct sprite_tolldoor*)sprite)

static int _tolldoor_init(struct sprite *sprite) {
  SPRITE->tollid=sprite->arg>>24;
  if ((SPRITE->tollid>=g.tollc)||!g.tollv[SPRITE->tollid]) return -1;
  sprite->solid=1;
  return 0;
}

static void _tolldoor_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->tollid>=g.tollc)||!g.tollv[SPRITE->tollid]) sprite->defunct=1;
}

static void _tolldoor_bump(struct sprite *sprite) {
  if ((SPRITE->tollid>=g.tollc)||!g.tollv[SPRITE->tollid]) { sprite->defunct=1; return; }
  if (g.gold<g.tollv[SPRITE->tollid]) {
    egg_play_sound(RID_sound_walk_reject);
  } else {
    egg_play_sound(RID_sound_pay);
    g.gold-=g.tollv[SPRITE->tollid];
    g.tollv[SPRITE->tollid]=0;
    sprite->defunct=1;
  }
}

static void _tolldoor_render(struct sprite *sprite,int x,int y) {
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,sprite->tileid,sprite->xform);
  int price=0;
  if (SPRITE->tollid<g.tollc) price=g.tollv[SPRITE->tollid];
  graf_set_tint(&g.graf,0x004000ff);
  graf_draw_tile(&g.graf,g.texid_sprites,x-4,y+3,0x30+price/10,0);
  graf_draw_tile(&g.graf,g.texid_sprites,x+3,y+3,0x30+price%10,0);
  graf_set_tint(&g.graf,0);
}

const struct sprite_type sprite_type_tolldoor={
  .name="tolldoor",
  .objlen=sizeof(struct sprite_tolldoor),
  .init=_tolldoor_init,
  .update=_tolldoor_update,
  .render=_tolldoor_render,
  .bump=_tolldoor_bump,
};

/* Add to visible tolls.
 */

void add_toll(int d) {
  int added=0;
  uint8_t donev[32]; // In real life, one doesn't expect multiple tolldoors on the same tollid, but allow it.
  int donec=0;
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type!=&sprite_type_tolldoor) continue;
    if (SPRITE->tollid>=g.tollc) continue;
    if (!g.tollv[SPRITE->tollid]) continue;
    int already=0;
    int j=donec; while (j-->0) if (donev[j]==SPRITE->tollid) {
      already=1;
      break;
    }
    if (already) continue;
    if (donec<32) donev[donec++]=SPRITE->tollid;
    if (g.tollv[SPRITE->tollid]<99) {
      if ((g.tollv[SPRITE->tollid]+=d)>99) {
        g.tollv[SPRITE->tollid]=99;
      }
      added=1;
    }
  }
  if (added) {
    egg_play_sound(RID_sound_add_toll);
  }
}
