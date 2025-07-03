#include "game/game.h"

struct sprite_tolldoor {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_tolldoor*)sprite)

static void _tolldoor_del(struct sprite *sprite) {
}

static int _tolldoor_init(struct sprite *sprite) {
  return 0;
}

static void _tolldoor_update(struct sprite *sprite,double elapsed) {
}

static void _tolldoor_render(struct sprite *sprite,int x,int y) {
}

const struct sprite_type sprite_type_tolldoor={
  .name="tolldoor",
  .objlen=sizeof(struct sprite_tolldoor),
  //.del=_tolldoor_del,
  //.init=_tolldoor_init,
  //.update=_tolldoor_update,
  //.render=_tolldoor_render,
};
