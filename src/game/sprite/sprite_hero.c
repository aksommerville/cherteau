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

/* Begin step.
 */
 
static void begin_step(struct sprite *sprite,int dx,int dy) {
  SPRITE->walking=1;
  SPRITE->qx+=dx;
  SPRITE->qy+=dy;
  SPRITE->dstx=SPRITE->qx+0.5;
  SPRITE->dsty=SPRITE->qy+0.5;
  if ((SPRITE->qx>=0)&&(SPRITE->qy>=0)&&(SPRITE->qx<NS_sys_mapw)&&(SPRITE->qy<NS_sys_maph)) {
    uint8_t physics=g.physics[g.map->cellv[SPRITE->qy*NS_sys_mapw+SPRITE->qx]];
    if (physics==NS_physics_safe) return;
  }
  add_toll(1);
}

static void end_step(struct sprite *sprite) {
  sprite->x=SPRITE->dstx;
  sprite->y=SPRITE->dsty;
  SPRITE->walking=0;
  const struct rom_command *poi=g.poiv;
  int i=g.poic;
  for (;i-->0;poi++) {
    if (poi->argv[0]!=SPRITE->qx) continue;
    if (poi->argv[1]!=SPRITE->qy) continue;
    switch (poi->opcode) {
      case CMD_map_gameover: {
          fprintf(stderr,"!!! Game over !!! %s:%d\n",__FILE__,__LINE__);
        } break;
      case CMD_map_treasure: {
          int trid=(poi->argv[2]<<8)|poi->argv[3];
          if ((trid<g.treasurec)&&g.treasurev[trid]) {
            if ((g.gold+=g.treasurev[trid])>999) g.gold=999;
            g.treasurev[trid]=0;
            g.map->cellv[SPRITE->qy*NS_sys_mapw+SPRITE->qx]=g.map->rocellv[SPRITE->qy*NS_sys_mapw+SPRITE->qx]+1;
            egg_play_sound(RID_sound_gold);
            //TODO dialogue box
          }
        } break;
      case CMD_map_treadle1: {
          int flagid=(poi->argv[2]<<8)|poi->argv[3];
          if (set_flag(flagid,1)) egg_play_sound(RID_sound_treadle);
        } break;
    }
  }
}

/* Check encounters.
 * If it's time for one, trigger it and return nonzero.
 */
 
static int check_encounters(struct sprite *sprite) {
  if ((SPRITE->qx>=0)&&(SPRITE->qy>=0)&&(SPRITE->qx<NS_sys_mapw)&&(SPRITE->qy<NS_sys_maph)) {
    uint8_t physics=g.physics[g.map->cellv[SPRITE->qy*NS_sys_mapw+SPRITE->qx]];
    if (physics==NS_physics_safe) return 0;
  }
  if ((rand()&0xffff)<g.encodds) {
    g.begin_encounter=1;
    g.encodds=g.encodds0;
    return 1;
  }
  g.encodds+=g.encoddsd;
  return 0;
}

/* Update.
 */

static void _hero_update(struct sprite *sprite,double elapsed) {

  struct sprite *block=0;
  if (SPRITE->walking) {
    #define AXISWALK(axis,dir,termcmp,btntag,dx,dy) { \
      sprite->axis+=dir*WALK_SPEED*elapsed; \
      if (sprite->axis termcmp SPRITE->dst##axis) { \
        if (g.input&EGG_BTN_##btntag) { \
          if (check_encounters(sprite)) { \
            end_step(sprite); \
          } else if (walk_ok(sprite,dx,dy,&block)) { \
            begin_step(sprite,dx,dy); \
          } else { \
            end_step(sprite); \
            if (block&&block->type->bump) block->type->bump(block); \
            else egg_play_sound(RID_sound_walk_reject); \
          } \
        } else { \
          end_step(sprite); \
          check_encounters(sprite); \
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
        begin_step(sprite,-1,0);
      } else if (!(g.pvinput&EGG_BTN_LEFT)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
    case EGG_BTN_RIGHT: SPRITE->facedir=0x08; if (walk_ok(sprite,1,0,&block)) {
        begin_step(sprite,1,0);
      } else if (!(g.pvinput&EGG_BTN_RIGHT)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
    case EGG_BTN_UP: SPRITE->facedir=0x40; if (walk_ok(sprite,0,-1,&block)) {
        begin_step(sprite,0,-1);
      } else if (!(g.pvinput&EGG_BTN_UP)) {
        if (block&&block->type->bump) block->type->bump(block);
        else egg_play_sound(RID_sound_walk_reject);
      }
      break;
    case EGG_BTN_DOWN: SPRITE->facedir=0x02; if (walk_ok(sprite,0,1,&block)) {
        begin_step(sprite,0,1);
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
