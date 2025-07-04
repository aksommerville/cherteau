/* modal.h
 */
 
#ifndef MODAL_H
#define MODAL_H

struct modal;
struct modal_type;

struct modal {
  const struct modal_type *type;
  int opaque; // If zero, we'll draw the world first.
};

struct modal_type {
  const char *name;
  int objlen;
  void (*del)(struct modal *modal);
  int (*init)(struct modal *modal);
  void (*input)(struct modal *modal);
  int (*update)(struct modal *modal,double elapsed); // <=0 to finish
  void (*render)(struct modal *modal);
};

void modal_del(struct modal *modal);
struct modal *modal_new(const struct modal_type *type);

extern const struct modal_type modal_type_encounter;

#endif
