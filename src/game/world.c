/* world.c
 * Responsible for the outer-world gameplay mode.
 */

#include "game.h"

static void world_render_diegetic(int dstx,int dsty,int skip_hero);

/* Redraw minimap.
 */
 
static uint32_t minimap_tmp[MINIMAP_COLW*NS_sys_worldw*MINIMAP_ROWH*NS_sys_worldh];

static void fill_rect(uint32_t *dstrow,int stride,int w,int h,uint32_t color) {
  for (;h-->0;dstrow+=stride) {
    uint32_t *dstp=dstrow;
    int xi=w;
    for (;xi-->0;dstp++) *dstp=color;
  }
}

static int has_vacancy(const struct map *map,int x,int y,int w,int h) {
  const uint8_t *rowp=map->cellv+y*NS_sys_mapw+x;
  for (;h-->0;rowp+=NS_sys_mapw) {
    const uint8_t *p=rowp;
    int xi=w;
    for (;xi-->0;p++) switch (g.physics[*p]) {
      case NS_physics_vacant:
      case NS_physics_safe:
        return 1;
    }
  }
  return 0;
}

static int has_neighbor(const struct map *map,int dx,int dy) {
  // If the neighbor doesn't exist in g.maps_by_location, that's an easy "no".
  int lng=map->lng+dx; if ((lng<0)||(lng>=NS_sys_worldw)) return 0;
  int lat=map->lat+dy; if ((lat<0)||(lat>=NS_sys_worldh)) return 0;
  // Tempting to leave it at that, but that would have us drawing doors where two neighbors are in fact not joined.
  if ((dx<0)&&!has_vacancy(map,0,0,1,NS_sys_maph)) return 0;
  if ((dx>0)&&!has_vacancy(map,NS_sys_mapw-1,0,1,NS_sys_maph)) return 0;
  if ((dy<0)&&!has_vacancy(map,0,0,NS_sys_mapw,1)) return 0;
  if ((dy>0)&&!has_vacancy(map,0,NS_sys_maph-1,NS_sys_mapw,1)) return 0;
  return 1;
}

static void redraw_minimap_1(uint32_t *dstrow,const struct map *map) {
  int stride=MINIMAP_COLW*NS_sys_worldw; // stride in words
  fill_rect(dstrow,stride,MINIMAP_COLW,MINIMAP_ROWH,0x00000000);
  if (!map||!map->visited) return;
  const uint32_t roomcolor=0xff808080;
  const uint32_t doorcolor=0xffa0a0a0;
  fill_rect(dstrow+stride+1,stride,MINIMAP_COLW-2,MINIMAP_ROWH-2,roomcolor);
  int mblp=map->lat*NS_sys_worldw+map->lng;
  if (has_neighbor(map,-1,0)) fill_rect(dstrow+(MINIMAP_ROWH>>1)*stride,0,1,1,doorcolor);
  if (has_neighbor(map,1,0)) fill_rect(dstrow+MINIMAP_COLW-1+(MINIMAP_ROWH>>1)*stride,0,1,1,doorcolor);
  if (has_neighbor(map,0,-1)) fill_rect(dstrow+(MINIMAP_COLW>>1),0,1,1,doorcolor);
  if (has_neighbor(map,0,1)) fill_rect(dstrow+(MINIMAP_COLW>>1)+(MINIMAP_ROWH-1)*stride,0,1,1,doorcolor);
}
 
static void redraw_minimap() {
  struct map **mapp=g.maps_by_location;
  uint32_t *dstrow=minimap_tmp;
  int w=MINIMAP_COLW*NS_sys_worldw;
  int h=MINIMAP_ROWH*NS_sys_worldh;
  int longstride=MINIMAP_COLW*NS_sys_worldw*MINIMAP_ROWH;
  int shortstride=MINIMAP_COLW;
  int rowi=NS_sys_worldh;
  for (;rowi-->0;dstrow+=longstride) {
    uint32_t *dstp=dstrow;
    int coli=NS_sys_worldw;
    for (;coli-->0;dstp+=shortstride,mapp++) {
      redraw_minimap_1(dstp,*mapp);
    }
  }
  egg_texture_load_raw(g.texid_minimap,w,h,w*4,minimap_tmp,sizeof(minimap_tmp));
}

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
  
  if (!map->visited) {
    map->visited=1;
    redraw_minimap();
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
    if ((sprite->type==&sprite_type_hero)&&!sprite->defunct) {
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
  g.encodds=g.encodds0=0;
  g.encoddsd=0;
  g.begin_encounter=0;
  g.poic=0;
  
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_gameover: {
          uint8_t x=cmd.argv[0],y=cmd.argv[1];
          if ((x<NS_sys_mapw)&&(y<NS_sys_maph)&&(g.poic<POI_LIMIT)) {
            g.poiv[g.poic++]=cmd;
          }
        } break;
      case CMD_map_encodds: {
          g.encodds0=(cmd.argv[0]<<8)|cmd.argv[1];
          if (g.encodds0&0x8000) g.encodds0|=~0xffff;
          g.encoddsd=(cmd.argv[2]<<8)|cmd.argv[3];
        } break;
      case CMD_map_treasure: {
          uint8_t x=cmd.argv[0],y=cmd.argv[1];
          if ((x<NS_sys_mapw)&&(y<NS_sys_maph)) {
            int trid=(cmd.argv[2]<<8)|cmd.argv[3];
            if ((trid<g.treasurec)&&g.treasurev[trid]) {
              map->cellv[y*NS_sys_mapw+x]=map->rocellv[y*NS_sys_mapw+x];
              if (g.poic<POI_LIMIT) {
                g.poiv[g.poic++]=cmd;
              }
            } else {
              map->cellv[y*NS_sys_mapw+x]=map->rocellv[y*NS_sys_mapw+x]+1;
            }
          }
        } break;
      case CMD_map_treadle1: {
          uint8_t x=cmd.argv[0],y=cmd.argv[1];
          int flagid=(cmd.argv[2]<<8)|cmd.argv[3];
          if ((x<NS_sys_mapw)&&(y<NS_sys_maph)) {
            if ((flagid<FLAG_COUNT)&&g.flagv[flagid]) {
              map->cellv[y*NS_sys_mapw+x]=map->rocellv[y*NS_sys_mapw+x]+1;
            } else {
              map->cellv[y*NS_sys_mapw+x]=map->rocellv[y*NS_sys_mapw+x];
              if (g.poic<POI_LIMIT) {
                g.poiv[g.poic++]=cmd;
              }
            }
          }
        } break;
      case CMD_map_switchable: {
          uint8_t x=cmd.argv[0],y=cmd.argv[1];
          int flagid=(cmd.argv[2]<<8)|cmd.argv[3];
          if ((x<NS_sys_mapw)&&(y<NS_sys_maph)) {
            if ((flagid<FLAG_COUNT)&&g.flagv[flagid]) {
              map->cellv[y*NS_sys_mapw+x]=map->rocellv[y*NS_sys_mapw+x]+1;
            } else {
              map->cellv[y*NS_sys_mapw+x]=map->rocellv[y*NS_sys_mapw+x];
            }
          }
        } break;
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
  g.encodds=g.encodds0;
  
  return 0;
}

/* Reset.
 */
 
int world_reset() {
  g.hp=5;
  g.maxhp=5;
  g.gold=0;
  memset(g.flagv,0,sizeof(g.flagv));
  g.flagv[1]=1;
  
  struct map *map=g.mapv;
  int i=g.mapc;
  for (;i-->0;map++) {
    memcpy(map->cellv,map->rocellv,sizeof(map->cellv));
    map->visited=0;
  }
  
  // TODO Treasures and tolls probably shouldn't all be the same value initially.
  memset(g.tollv,10,TOLL_LIMIT);
  g.tollc=TOLL_LIMIT;
  memset(g.treasurev,100,TREASURE_LIMIT);
  g.treasurec=TREASURE_LIMIT;
  
  if (g.hero) g.hero->defunct=1; // Normally enter_map() preserves the hero sprite. We want it dead.
  egg_play_song(RID_song_lock_me_away,0,1);
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
  int mmw=MINIMAP_COLW*NS_sys_worldw;
  int mmh=MINIMAP_ROWH*NS_sys_worldh;
  dstx=FBW-mmw;
  dsty=0;
  graf_draw_decal(&g.graf,g.texid_minimap,dstx,dsty,0,0,mmw,mmh,0);
  dstx+=g.map->lng*MINIMAP_COLW+1;
  dsty+=g.map->lat*MINIMAP_ROWH+1;
  graf_draw_rect(&g.graf,dstx,dsty,MINIMAP_COLW-2,MINIMAP_ROWH-2,(g.framec&16)?0xffc020ff:0xff8000ff);
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

/* Set flag, with side effects and everything.
 */
 
int set_flag(int flagid,int v) {
  if ((flagid<2)||(flagid>=FLAG_COUNT)) return 0; // sic "<2", the first 2 are read-only
  if (g.flagv[flagid]==v) return 0;
  g.flagv[flagid]=v;
  
  if (!g.map) return 1;
  struct rom_command_reader reader={.v=g.map->cmdv,.c=g.map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_treadle1:
      case CMD_map_switchable: {
          int cflagid=(cmd.argv[2]<<8)|cmd.argv[3];
          if (cflagid!=flagid) break;
          uint8_t x=cmd.argv[0],y=cmd.argv[1];
          if ((x<NS_sys_mapw)&&(y<NS_sys_maph)) {
            if (v) {
              g.map->cellv[y*NS_sys_mapw+x]=g.map->rocellv[y*NS_sys_mapw+x]+1;
            } else {
              g.map->cellv[y*NS_sys_mapw+x]=g.map->rocellv[y*NS_sys_mapw+x];
            }
          }
        } break;
    }
  }
  return 1;
}
