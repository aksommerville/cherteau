/* sprite_flycoin.c
 * Animation for after you've paid a tolldoor.
 * There may be multiple flying coins, but they're all managed by one sprite.
 */
 
#include "game/game.h"

struct sprite_flycoin {
  struct sprite hdr;
  double radius; // pixels
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_flycoin*)sprite)

static int _flycoin_init(struct sprite *sprite) {
  return 0;
}

static void _flycoin_update(struct sprite *sprite,double elapsed) {
  SPRITE->radius+=elapsed*50.0;
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.125;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
  if (SPRITE->radius>=FBH) sprite->defunct=1;
}

static void flycoin_render_1(struct sprite *sprite,double x,double y,double t,uint8_t tileid) {
  x+=sin(t)*SPRITE->radius;
  y-=cos(t)*SPRITE->radius;
  graf_draw_tile(&g.graf,g.texid_sprites,(int)x,(int)y,tileid,0);
}

static void _flycoin_render(struct sprite *sprite,int x,int y) {
  double fx=x,fy=y;
  uint8_t tileid=0x81;
  switch (SPRITE->animframe) {
    case 1: case 3: tileid+=1; break;
    case 2: tileid+=2; break;
  }
  flycoin_render_1(sprite,fx,fy,-1.0,tileid);
  flycoin_render_1(sprite,fx,fy,0.0,tileid);
  flycoin_render_1(sprite,fx,fy,1.0,tileid);
}

const struct sprite_type sprite_type_flycoin={
  .name="flycoin",
  .objlen=sizeof(struct sprite_flycoin),
  .init=_flycoin_init,
  .update=_flycoin_update,
  .render=_flycoin_render,
};
