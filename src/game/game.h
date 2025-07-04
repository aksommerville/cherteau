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

#define FBW 320
#define FBH 180

#define TOLL_LIMIT 64

#define TRANSITION_PAN_LEFT 1
#define TRANSITION_PAN_RIGHT 2
#define TRANSITION_PAN_UP 3
#define TRANSITION_PAN_DOWN 4

#define TRANSITION_TIME 0.400

struct map {
  uint16_t rid;
  uint8_t lng,lat;
  const uint8_t *rocellv;
  uint8_t cellv[NS_sys_mapw*NS_sys_maph];
  const uint8_t *cmdv;
  int cmdc;
};

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int texid_castle;
  int texid_sprites;
  uint8_t physics[256];
  int framec;
  int input,pvinput;
  
  struct map *maps_by_location[NS_sys_worldw*NS_sys_worldh];
  struct map *mapv; // by id; all resources, including invalid locations
  int mapc,mapa;
  
  struct map *map; // WEAK, present during play.
  int hp,maxhp;
  int gold; // 0..999
  struct sprite **spritev;
  int spritec,spritea;
  struct sprite *hero; // WEAK
  uint8_t tollv[TOLL_LIMIT]; // 0..99. Once they hit zero they stay zero.
  int tollc;
  int transition;
  double transition_clock;
  int transition_bits; // texture
} g;

struct map *mapv_get(int rid);
int res_get(void *dstpp,int tid,int rid);

int world_reset();
void world_render();
void check_transitions();

/* Add (d) to any tolls currently visible.
 */
void add_toll(int d);

#endif
