/* sprite.h
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

struct sprite {
  const struct sprite_type *type;
  int defunct;
  double x,y;
  uint8_t tileid,xform;
  uint32_t arg;
  const uint8_t *cmdv;
  int cmdc;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  void (*render)(struct sprite *sprite,int x,int y);
};

/* Prefer not to use del or new.
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(const struct sprite_type *type,double x,double y,uint32_t arg,const void *cmdv,int cmdc);

/* Provide one of (rid,type).
 * Returns WEAK.
 */
struct sprite *sprite_spawn(int rid,const struct sprite_type *type,double x,double y,uint32_t arg);

const struct sprite_type *sprite_type_by_id(int id);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
FOR_EACH_SPRTYPE
#undef _

#endif
