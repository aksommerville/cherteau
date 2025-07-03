#ifndef GAME_H
#define GAME_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"

#define FBW 320
#define FBH 180

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
} g;

#endif
