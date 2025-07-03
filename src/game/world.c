/* world.c
 * Responsible for the outer-world gameplay mode.
 */

#include "game.h"

/* Load map etc etc.
 * Don't do this during a sprite update.
 */
 
static int enter_map(int rid) {
  struct map *map=mapv_get(rid);
  if (!map) {
    fprintf(stderr,"map:%d not found\n",rid);
    return -1;
  }
  
  while (g.spritec>0) {
    g.spritec--;
    sprite_del(g.spritev[g.spritec]);
  }
  
  g.map=map;
  
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_sprite: {
          double x=cmd.argv[0]+0.5;
          double y=cmd.argv[1]+0.5;
          int rid=(cmd.argv[2]<<8)|cmd.argv[3];
          uint32_t arg=(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7];
          struct sprite *sprite=sprite_spawn(rid,0,x,y,arg);
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
  if (enter_map(RID_map_start)<0) return -1;
  return 0;
}

/* Render.
 */
 
void world_render() {
  int dstx,dsty,i;
  const int HEADERH=(FBH-NS_sys_maph*NS_sys_tilesize);
  
  // Map.
  dstx=NS_sys_tilesize>>1;
  dsty=HEADERH+(NS_sys_tilesize>>1);
  graf_draw_tile_buffer(&g.graf,g.texid_castle,dstx,dsty,g.map->cellv,NS_sys_mapw,NS_sys_maph,NS_sys_mapw);
  dstx-=NS_sys_tilesize>>1;
  dsty-=NS_sys_tilesize>>1;
  
  // Sprites.
  struct sprite **p=g.spritev;
  for (i=g.spritec;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    int sx=dstx+(int)(sprite->x*NS_sys_tilesize);
    int sy=dsty+(int)(sprite->y*NS_sys_tilesize);
    if (sprite->type->render) {
      sprite->type->render(sprite,sx,sy);
    } else {
      graf_draw_tile(&g.graf,g.texid_sprites,sx,sy,sprite->tileid,sprite->xform);
    }
  }

  // Blackout header zone, draw HP and gold at the left side.
  graf_draw_rect(&g.graf,0,0,FBW,HEADERH,0x000000ff);
  dstx=5;
  dsty=5;
  for (i=0;i<g.maxhp;i++,dstx+=8) graf_draw_tile(&g.graf,g.texid_sprites,dstx,dsty,(i<g.hp)?0x3b:0x3a,0);
  graf_draw_tile(&g.graf,g.texid_sprites,5,15,0x3c,0);
  graf_draw_tile(&g.graf,g.texid_sprites,16,15,0x31,0);
  graf_draw_tile(&g.graf,g.texid_sprites,23,15,0x32,0);
  graf_draw_tile(&g.graf,g.texid_sprites,30,15,0x33,0);
  
  // Minimap on the right side of the header.
  dstx=FBW-1-NS_sys_worldw;
  dsty=1;
  graf_draw_rect(&g.graf,dstx,dsty,NS_sys_worldw,NS_sys_worldh,0x404040ff);
  graf_draw_rect(&g.graf,dstx+g.map->lng,dsty+g.map->lat,1,1,(g.framec&16)?0xffffffff:0xff8000ff);
}
