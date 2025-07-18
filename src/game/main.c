#include "game.h"

struct g g={0};

/* Resource store.
 *********************************************************************************/
 
int res_get(void *dstpp,int tid,int rid) {
  struct rom_reader reader={0};
  rom_reader_init(&reader,g.rom,g.romc);
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid>tid) return 0;
    if (res->tid<tid) continue;
    if (res->rid>rid) return 0;
    if (res->rid<rid) continue;
    *(const void**)dstpp=res->v;
    return res->c;
  }
  return 0;
}
 
static int mapv_search(int rid) {
  int lo=0,hi=g.mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=g.mapv[ck].rid;
         if (rid<q) hi=ck;
    else if (rid>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

struct map *mapv_get(int rid) {
  int p=mapv_search(rid);
  if (p<0) return 0;
  return g.mapv+p;
}
 
static int add_map(int rid,const void *v,int c) {
  int p=mapv_search(rid);
  if (p>=0) return -1;
  p=-p-1;
  if (g.mapc>=g.mapa) {
    int na=g.mapa+32;
    if (na>INT_MAX/sizeof(struct map)) return -1;
    void *nv=realloc(g.mapv,sizeof(struct map)*na);
    if (!nv) return -1;
    g.mapv=nv;
    g.mapa=na;
  }
  struct map *map=g.mapv+p;
  memmove(map+1,map,sizeof(struct map)*(g.mapc-p));
  memset(map,0,sizeof(struct map));
  g.mapc++;
  map->rid=rid;
  struct rom_map rmap={0};
  if (rom_map_decode(&rmap,v,c)<0) return -1;
  if ((rmap.w!=NS_sys_mapw)||(rmap.h!=NS_sys_maph)) {
    fprintf(stderr,"map:%d: Size %dx%d, must be %dx%d\n",rid,rmap.w,rmap.h,NS_sys_mapw,NS_sys_maph);
    return -1;
  }
  map->rocellv=rmap.v;
  map->cmdv=rmap.cmdv;
  map->cmdc=rmap.cmdc;
  memcpy(map->cellv,map->rocellv,NS_sys_mapw*NS_sys_maph);
  
  struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_location: {
          if (cmd.argv[2]) break; // Nonzero 'z'. Ignore it.
          int lng=cmd.argv[0];
          int lat=cmd.argv[1];
          if ((lng>=NS_sys_worldw)||(lat>=NS_sys_worldh)) {
            fprintf(stderr,"map:%d at %d,%d in world; limit %d,%d\n",rid,lat,lng,NS_sys_worldw,NS_sys_worldh);
            return -1;
          }
          map->lat=lat;
          map->lng=lng;
          g.maps_by_location[lat*NS_sys_worldw+lng]=map;
        } break;
    }
  }
  
  return 0;
}

static int add_tilesheet(const void *v,int c) {
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,v,c)<0) return -1;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid!=NS_tilesheet_physics) continue;
    memcpy(g.physics+entry.tileid,entry.v,entry.c);
  }
  return 0;
}

/* Platform entry points.
 ***********************************************************************************/

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"got fb %dx%d (metadata), expected %dx%d (game.h)\n",fbw,fbh,FBW,FBH);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  if (egg_texture_load_image(g.texid_castle=egg_texture_new(),RID_image_castle)<0) return -1;
  if (egg_texture_load_image(g.texid_sprites=egg_texture_new(),RID_image_sprites)<0) return -1;
  if (egg_texture_load_image(g.texid_tilefont=egg_texture_new(),RID_image_tilefont)<0) return -1;
  if (egg_texture_load_raw(g.texid_minimap=egg_texture_new(),
    MINIMAP_COLW*NS_sys_worldw,MINIMAP_ROWH*NS_sys_worldh,
    MINIMAP_COLW*NS_sys_worldw*4,0,0
  )<0) return -1;
  
  struct rom_reader reader={0};
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    switch (res->tid) {
      case EGG_TID_map: if (add_map(res->rid,res->v,res->c)<0) return -1; break;
      case EGG_TID_tilesheet: if (res->rid==RID_image_castle) add_tilesheet(res->v,res->c); break;
    }
  }
  
  srand_auto();
  
  stats_load(&g.stats_best);
  
  g.modal=modal_new(&modal_type_hello);
  
  return 0;
}

void egg_client_update(double elapsed) {
  g.pvinput=g.input;
  g.input=egg_input_get_one(0);
  if (g.input!=g.pvinput) {
    if ((g.input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) {
      if (!g.modal||(g.modal->type!=&modal_type_hello)) {
        modal_del(g.modal);
        g.modal=modal_new(&modal_type_hello);
      }
    }
    if (g.modal&&g.modal->type->input) g.modal->type->input(g.modal);
  }
  if (g.modal) {
    if (g.modal->type==&modal_type_encounter) { // playtime does advance during battle. Not during other modals.
      g.playtime+=elapsed;
    }
    if (!g.modal->type->update||(g.modal->type->update(g.modal,elapsed)<=0)) {
      int tohello=(g.modal->type==&modal_type_gameover)||(g.hp<=0);
      modal_del(g.modal);
      g.modal=0;
      if (tohello) {
        g.modal=modal_new(&modal_type_hello);
      }
    }
  } else if (g.map) {
    g.playtime+=elapsed;
    int i=g.spritec;
    while (i-->0) {
      struct sprite *sprite=g.spritev[i];
      if (sprite->defunct) {
        g.spritec--;
        memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
        sprite_del(sprite);
      } else if (sprite->type->update) {
        sprite->type->update(sprite,elapsed);
      }
    }
    world_update_displayed_stats(elapsed);
    if (g.begin_encounter) {
      g.begin_encounter=0;
      egg_play_sound(RID_sound_encounter);
      g.modal=modal_new(&modal_type_encounter);
    } else {
      if ((g.transition_clock-=elapsed)<=0.0) g.transition=0;
      check_transitions();
      if (g.hp<=0) {
        modal_del(g.modal); // should be null already but be safe
        g.modal=modal_new(&modal_type_hello);
      }
    }
  }
}

void egg_client_render() {
  int dstx,dsty,i;
  g.framec++;
  graf_reset(&g.graf);
  if (g.modal) {
    if (g.map&&!g.modal->opaque) world_render();
    if (g.modal->type->render) g.modal->type->render(g.modal);
  } else if (g.map) {
    world_render();
  } else {
    graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  }
  graf_flush(&g.graf);
}
