/* world.c
 * Responsible for the outer-world gameplay mode.
 */

#include "game.h"

static void world_render_diegetic(int dstx,int dsty,int skip_hero);

/* Generate the transition texture if we need it, and draw the world into it.
 */
 
static void draw_transition_bits() {
  if (g.transition_bits<1) {
    if ((g.transition_bits=egg_texture_new())<1) return;
    egg_texture_load_raw(g.transition_bits,NS_sys_tilesize*NS_sys_mapw,NS_sys_tilesize*NS_sys_maph,NS_sys_tilesize*NS_sys_mapw*4,0,0);
  }
  graf_set_output(&g.graf,g.transition_bits);
  world_render_diegetic(0,0,1);
  graf_flush(&g.graf);
  graf_set_output(&g.graf,1);
}

/* Load map etc etc.
 * Don't do this during a sprite update.
 */
 
static int enter_map(int rid,struct map *map,int dx,int dy) {
  if (!map) {
    if (!(map=mapv_get(rid))) {
      fprintf(stderr,"map:%d not found\n",rid);
      return -1;
    }
  }
  
  g.transition=0;
  if (dx<0) g.transition=TRANSITION_PAN_LEFT;
  else if (dx>0) g.transition=TRANSITION_PAN_RIGHT;
  else if (dy<0) g.transition=TRANSITION_PAN_UP;
  else if (dy>0) g.transition=TRANSITION_PAN_DOWN;
  if (g.transition) {
    g.transition_clock=TRANSITION_TIME;
    draw_transition_bits();
  }
  
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->type==&sprite_type_hero) {
      sprite->x-=dx*NS_sys_mapw;
      sprite->y-=dy*NS_sys_maph;
      if (sprite->type->position_changed) sprite->type->position_changed(sprite);
      continue;
    }
    g.spritec--;
    memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
    sprite_del(sprite);
  }
  
  g.map=map;
  
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_sprite: {
          double x=cmd.argv[0]+0.5;
          double y=cmd.argv[1]+0.5;
          int srid=(cmd.argv[2]<<8)|cmd.argv[3];
          if ((srid==RID_sprite_hero)&&g.hero) break;
          uint32_t arg=(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7];
          struct sprite *sprite=sprite_spawn(srid,0,x,y,arg);
        } break;
      case CMD_map_door: {
          //TODO door
        } break;
    }
  }
  
  return 0;
}

/* Reset.
 */
 
int world_reset() {
  g.hp=4;
  g.maxhp=8;
  g.gold=400;
  memset(g.tollv,10,TOLL_LIMIT);
  g.tollc=TOLL_LIMIT;
  if (enter_map(RID_map_start,0,0,0)<0) return -1;
  return 0;
}

/* Render just the map and sprites.
 */
 
static void world_render_diegetic(int dstx,int dsty,int skip_hero) {
  int i;
  
  graf_draw_tile_buffer(&g.graf,g.texid_castle,dstx+(NS_sys_tilesize>>1),dsty+(NS_sys_tilesize>>1),g.map->cellv,NS_sys_mapw,NS_sys_maph,NS_sys_mapw);
  
  struct sprite **p=g.spritev;
  for (i=g.spritec;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    if (skip_hero&&(sprite->type==&sprite_type_hero)) continue;
    int sx=dstx+(int)(sprite->x*NS_sys_tilesize);
    int sy=dsty+(int)(sprite->y*NS_sys_tilesize);
    if (sprite->type->render) {
      sprite->type->render(sprite,sx,sy);
    } else {
      graf_draw_tile(&g.graf,g.texid_sprites,sx,sy,sprite->tileid,sprite->xform);
    }
  }
}

/* Render the diegetic world and transition_bits, applying a transition.
 * Pan (d) are the camera's direction of movement.
 */
 
static void world_render_pan(int dstx,int dsty,double dx,double dy) {
  double t=g.transition_clock/TRANSITION_TIME; // reverse, so now (d) is the direction the real world moves
  const int fbw=(NS_sys_mapw*NS_sys_tilesize);
  const int fbh=(NS_sys_maph*NS_sys_tilesize);
  int ndstx=(int)(dstx+t*dx*fbw);
  int ndsty=(int)(dsty+t*dy*fbh);
  int odstx=ndstx-dx*fbw;
  int odsty=ndsty-dy*fbh;
  world_render_diegetic(ndstx,ndsty,0);
  graf_draw_decal(&g.graf,g.transition_bits,odstx,odsty,0,0,fbw,fbh,0);
}

/* Render.
 */
 
void world_render() {
  int dstx,dsty,i;
  const int HEADERH=(FBH-NS_sys_maph*NS_sys_tilesize);
  
  switch (g.transition) {
    case 0: world_render_diegetic(0,HEADERH,0); break;
    case TRANSITION_PAN_LEFT: world_render_pan(0,HEADERH,-1.0,0.0); break;
    case TRANSITION_PAN_RIGHT: world_render_pan(0,HEADERH,1.0,0.0); break;
    case TRANSITION_PAN_UP: world_render_pan(0,HEADERH,0.0,-1.0); break;
    case TRANSITION_PAN_DOWN: world_render_pan(0,HEADERH,0.0,1.0); break;
  }

  // Blackout header zone, draw HP and gold at the left side.
  graf_draw_rect(&g.graf,0,0,FBW,HEADERH,0x000000ff);
  dstx=5;
  dsty=5;
  for (i=0;i<g.maxhp;i++,dstx+=8) graf_draw_tile(&g.graf,g.texid_sprites,dstx,dsty,(i<g.hp)?0x3b:0x3a,0);
  graf_draw_tile(&g.graf,g.texid_sprites,5,15,0x3c,0);
  graf_draw_tile(&g.graf,g.texid_sprites,16,15,0x30+g.gold/100,0);
  graf_draw_tile(&g.graf,g.texid_sprites,23,15,0x30+(g.gold/10)%10,0);
  graf_draw_tile(&g.graf,g.texid_sprites,30,15,0x30+g.gold%10,0);
  
  // Minimap on the right side of the header.
  dstx=FBW-1-NS_sys_worldw;
  dsty=1;
  graf_draw_rect(&g.graf,dstx,dsty,NS_sys_worldw,NS_sys_worldh,0x404040ff);
  graf_draw_rect(&g.graf,dstx+g.map->lng,dsty+g.map->lat,1,1,(g.framec&16)?0xffffffff:0xff8000ff);
}

/* Check transitions.
 */
 
static int check_transition_1(int dx,int dy) {
  int lng=g.map->lng+dx; if ((lng<0)||(lng>=NS_sys_worldw)) return 0;
  int lat=g.map->lat+dy; if ((lat<0)||(lat>=NS_sys_worldh)) return 0;
  struct map *map=g.maps_by_location[lat*NS_sys_worldw+lng];
  if (!map) return 0;
  if (enter_map(map->rid,map,dx,dy)<0) return 0;
  return 1;
}
 
void check_transitions() {
  if (!g.hero) return;
  if (!g.map) return;
  if ((g.hero->x<0.0)&&check_transition_1(-1,0)) return;
  if ((g.hero->x>NS_sys_mapw)&&check_transition_1(1,0)) return;
  if ((g.hero->y<0.0)&&check_transition_1(0,-1)) return;
  if ((g.hero->y>NS_sys_maph)&&check_transition_1(0,1)) return;
}
