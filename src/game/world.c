/* world.c
 * Responsible for the outer-world gameplay mode.
 */

#include "game.h"

/* Reset.
 */
 
int world_reset() {
  if (!(g.map=mapv_get(RID_map_start))) return -1;
  g.hp=4;
  g.maxhp=8;
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
  graf_draw_tile(&g.graf,g.texid_sprites,dstx+NS_sys_tilesize*10+(NS_sys_tilesize>>1),dsty+NS_sys_tilesize*2+(NS_sys_tilesize>>1),0x00,0);

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
