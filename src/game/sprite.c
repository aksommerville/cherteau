#include "game.h"

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* New.
 */
 
struct sprite *sprite_new(const struct sprite_type *type,double x,double y,uint32_t arg,const void *cmdv,int cmdc) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  sprite->type=type;
  sprite->x=x;
  sprite->y=y;
  sprite->arg=arg;
  sprite->cmdv=cmdv;
  sprite->cmdc=cmdc;
  
  struct rom_command_reader reader={.v=cmdv,.c=cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_sprite_tile: {
          sprite->tileid=cmd.argv[0];
          sprite->xform=cmd.argv[1];
        } break;
    }
  }
  
  if (type->init&&(type->init(sprite)<0)) {
    sprite_del(sprite);
    return 0;
  }
  
  return sprite;
}

/* Spawn.
 */

struct sprite *sprite_spawn(int rid,const struct sprite_type *type,double x,double y,uint32_t arg) {
  const uint8_t *cmdv=0;
  int cmdc=0;
  if (rid) {
    const uint8_t *serial=0;
    int serialc=res_get(&serial,EGG_TID_sprite,rid);
    if (serialc<4) {
      fprintf(stderr,"sprite:%d not found\n",rid);
      return 0;
    }
    cmdv=serial+4;
    cmdc=serialc-4;
    struct rom_command_reader reader={.v=cmdv,.c=cmdc};
    struct rom_command cmd;
    while (rom_command_reader_next(&cmd,&reader)>0) {
      if (cmd.opcode==CMD_sprite_sprtype) {
        int stid=(cmd.argv[0]<<8)|cmd.argv[1];
        if (!(type=sprite_type_by_id(stid))) {
          fprintf(stderr,"sprtype %d not found, for sprite:%d\n",stid,rid);
          return 0;
        }
      }
    }
    if (!type) {
      fprintf(stderr,"sprite:%d does not declare 'sprtype'\n",rid);
      return 0;
    }
  } else if (!type) return 0;
  
  if (g.spritec>=g.spritea) {
    int na=g.spritea+32;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(g.spritev,sizeof(void*)*na);
    if (!nv) return 0;
    g.spritev=nv;
    g.spritea=na;
  }
  struct sprite *sprite=sprite_new(type,x,y,arg,cmdv,cmdc);
  if (!sprite) return 0;
  g.spritev[g.spritec++]=sprite;
  return sprite;
}

/* Type by id.
 */
 
const struct sprite_type *sprite_type_by_id(int id) {
  switch (id) {
    #define _(tag) case NS_sprtype_##tag: return &sprite_type_##tag;
    FOR_EACH_SPRTYPE
    #undef _
  }
  return 0;
}
