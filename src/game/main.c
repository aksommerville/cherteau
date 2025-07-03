#include "game.h"

struct g g={0};

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
  
  srand_auto();
  
  return 0;
}

void egg_client_update(double elapsed) {
  // TODO
}

void egg_client_render() {
  graf_reset(&g.graf);
  graf_flush(&g.graf);
}
