#include "game/game.h"

#define WALK_SPEED 5.0 /* m/s */

/* Instance definition.
 */

struct sprite_hero {
  struct sprite hdr;
  int qx,qy;
  double dstx,dsty;
  int walking;
  uint8_t facedir;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */

static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */

static int _hero_init(struct sprite *sprite) {
  SPRITE->qx=(int)sprite->x;
  SPRITE->qy=(int)sprite->y;
  return 0;
}

/* Can we take another step in the given direction?
 */
 
static int walk_ok(const struct sprite *sprite,int dx,int dy,struct sprite **obstruction) {
  int x=SPRITE->qx+dx,y=SPRITE->qy+dy;
  // OOB is ok, as long as the nearest cell is ok.
  if (x<0) x=0; else if (x>=NS_sys_mapw) x=NS_sys_mapw-1;
  if (y<0) y=0; else if (y>=NS_sys_maph) y=NS_sys_maph-1;
  uint8_t tileid=g.map->cellv[y*NS_sys_mapw+x];
  uint8_t physics=g.physics[tileid];
  switch (physics) {
    case NS_physics_solid:
      if (obstruction) *obstruction=0;
      return 0;
  }
  // Check solid sprites;
  int i=g.spritec;
  while (i-->0) {
    struct sprite *block=g.spritev[i];
    if (!block->solid) continue;
    if (block->defunct) continue;
    int bx=(int)block->x; if (bx!=x) continue;
    int by=(int)block->y; if (by!=y) continue;
    if (obstruction) *obstruction=block;
    return 0;
  }
  return 1;
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  struct sprite *block;
  if (SPRITE->walking) {
    #define AXISWALK(axis,dir,termcmp,btntag,dx,dy) { \
      sprite->axis+=dir*WALK_SPEED*elapsed; \
      if (sprite->axis termcmp SPRITE->dst##axis) { \
        if (g.input&EGG_BTN_##btntag) { \
          if (walk_ok(sprite,dx,dy,&block)) { \
            SPRITE->qx+=dx; \
            SPRITE->qy+=dy; \
            SPRITE->dstx+=dx; \
            SPRITE->dsty+=dy; \
            add_toll(1); \
          } else { \
            sprite->axis=SPRITE->dst##axis; \
            SPRITE->walking=0; \
            if (block&&block->type->bump) block->type->bump(block); \
            else egg_play_sound(RID_sound_walk_reject); \
          } \
        } else { \
          sprite->axis=SPRITE->dst##axis; \
          SPRITE->walking=0; \
        } \
      } \
    }
         if (sprite->x>SPRITE->dstx) AXISWALK(x,-1.0,<=,LEFT,-1,0)
    else if (sprite->x<SPRITE->dstx) AXISWALK(x,1.0,>=,RIGHT,1,0)
    else if (sprite->y>SPRITE->dsty) AXISWALK(y,-1.0,<=,UP,0,-1)
    else if (sprite->y<SPRITE->dsty) AXISWALK(y,1.0,>=,DOWN,0,1)
    else {
      SPRITE->walking=0;
    }
    #undef AXISWALK
  } else switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT|EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_LEFT: SPRITE->facedir=0x10; if (walk_ok(sprite,-1,0,&block)) {
        SPRITE->walking=1;
        SPRITE->dstx=sprite->x-1.0;
        SPRITE->dsty=sprite->y;
        SPRITE->qx--;
        add_toll(1);
      } else if (!(g.pvinput&EGG_BTN_LEFT)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
    case EGG_BTN_RIGHT: SPRITE->facedir=0x08; if (walk_ok(sprite,1,0,&block)) {
        SPRITE->walking=1;
        SPRITE->dstx=sprite->x+1.0;
        SPRITE->dsty=sprite->y;
        SPRITE->qx++;
        add_toll(1);
      } else if (!(g.pvinput&EGG_BTN_RIGHT)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
    case EGG_BTN_UP: SPRITE->facedir=0x40; if (walk_ok(sprite,0,-1,&block)) {
        SPRITE->walking=1;
        SPRITE->dstx=sprite->x;
        SPRITE->dsty=sprite->y-1.0;
        SPRITE->qy--;
        add_toll(1);
      } else if (!(g.pvinput&EGG_BTN_UP)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
    case EGG_BTN_DOWN: SPRITE->facedir=0x02; if (walk_ok(sprite,0,1,&block)) {
        SPRITE->walking=1;
        SPRITE->dstx=sprite->x;
        SPRITE->dsty=sprite->y+1.0;
        SPRITE->qy++;
        add_toll(1);
      } else if (!(g.pvinput&EGG_BTN_DOWN)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
  }
}

/* Position changed from external force.
 */
 
static void _hero_position_changed(struct sprite *sprite) {
  SPRITE->qx=(int)sprite->x;
  SPRITE->qy=(int)sprite->y;
  sprite->x=SPRITE->qx+0.5;
  sprite->y=SPRITE->qy+0.5;
  SPRITE->walking=0;
  if (SPRITE->walking) {
    switch (SPRITE->facedir) {
      case 0x40: SPRITE->qy--; SPRITE->dsty=SPRITE->qy+0.5; break;
      case 0x10: SPRITE->qx--; SPRITE->dstx=SPRITE->qx+0.5; break;
      case 0x08: SPRITE->qx++; SPRITE->dstx=SPRITE->qx+0.5; break;
      case 0x02: SPRITE->qy++; SPRITE->dsty=SPRITE->qy+0.5; break;
    }
  }
}

/* Render.
 */

static void _hero_render(struct sprite *sprite,int x,int y) {
  uint8_t tileid=sprite->tileid;
  uint8_t xform=0;
  switch (SPRITE->facedir) {
    case 0x40: tileid+=0x01; break;
    case 0x10: tileid+=0x02; break;
    case 0x08: tileid+=0x02; xform=EGG_XFORM_XREV; break;
    case 0x02: break;
  }
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,tileid,xform);
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
  .position_changed=_hero_position_changed,
};
