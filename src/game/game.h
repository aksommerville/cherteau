#ifndef GAME_H
#define GAME_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"
#include "sprite.h"
#include "modal.h"
#include "minigame.h"

#define FBW 320
#define FBH 180

#define TOLL_LIMIT 64
#define TREASURE_LIMIT 32
#define POI_LIMIT 32

#define TRANSITION_PAN_LEFT 1
#define TRANSITION_PAN_RIGHT 2
#define TRANSITION_PAN_UP 3
#define TRANSITION_PAN_DOWN 4

#define TRANSITION_TIME 0.400

/* Don't change arbitrarily.
 * (FBH-NS_sys_tilesize*NS_sys_maph)==20, and NS_sys_worldw is 4.
 * So 5 pixels vertically fits the header bar exactly. And we'll never touch the edge pixels of the minimap, so its outline is preserved too.
 * Horizontally, we have some slack. Keep it odd, and I guess aim for either 1x or 2x of the height.
 */
#define MINIMAP_COLW 9
#define MINIMAP_ROWH 5

struct map {
  uint16_t rid;
  uint8_t lng,lat;
  const uint8_t *rocellv;
  uint8_t cellv[NS_sys_mapw*NS_sys_maph];
  const uint8_t *cmdv;
  int cmdc;
  int visited;
};

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int texid_castle;
  int texid_sprites;
  int texid_tilefont;
  uint8_t physics[256];
  int framec;
  int input,pvinput;
  
  struct map *maps_by_location[NS_sys_worldw*NS_sys_worldh];
  struct map *mapv; // by id; all resources, including invalid locations
  int mapc,mapa;
  
  struct modal *modal; // If present, the outer world does not update.
  struct map *map; // WEAK, present during play.
  struct rom_command poiv[POI_LIMIT]; // Selected commands from this map.
  int texid_minimap;
  int poic;
  int hp,maxhp;
  int gold; // 0..999
  int disphp,dispgold; // What's displayed. world.c manages this; other clients should touch (hp,gold) directly.
  double disphp_clock;
  double dispgold_clock;
  double gold_sound_clock;
  struct sprite **spritev;
  int spritec,spritea;
  struct sprite *hero; // WEAK
  uint8_t tollv[TOLL_LIMIT]; // 0..99. Once they hit zero they stay zero.
  int tollc;
  uint8_t treasurev[TREASURE_LIMIT];
  int treasurec;
  int transition;
  double transition_clock;
  int transition_bits; // texture
  int encodds; // 0..65535 odds of an encounter on the next step. Can go OOB.
  int encoddsd; // Increase encodds on each step.
  int encodds0;
  double difflo,diffhi;
  int begin_encounter; // Hero sets this during her update to begin an encounter postcycle.
  int flagv[FLAG_COUNT];
  void *last_game; // Used by modal_encounter to prevent playing the same game twice in a row.
} g;

struct map *mapv_get(int rid);
int res_get(void *dstpp,int tid,int rid);

int world_reset();
void world_render();
void check_transitions();

/* Add (d) to any tolls currently visible.
 */
void add_toll(int d);

/* Break lines of text for display with (g.texid_tilefont).
 * (bounds) will be filled with a box covering the text, plus a tasteful margin.
 * We assume the entire framebuffer is available.
 */
struct rect { int x,y,w,h; };
int break_text_tiles(struct egg_draw_tile *vtxv,int vtxa,struct rect *bounds,const char *src,int srcc);

int set_flag(int flagid,int v); // => nonzero if changed

void world_update_displayed_stats(double elapsed);

#endif
