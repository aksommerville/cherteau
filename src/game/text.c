#include "game.h"

/* Break text into tiles.
 */
 
int break_text_tiles(struct egg_draw_tile *vtxv,int vtxa,struct rect *bounds,const char *src,int srcc) {
  const int colw=8,rowh=8; // Per font image.
  const int wlimit=(FBW*3)/4; // Arbitrary, but must be <=FBW.
  int vtxc=0;
  int srcp=0;
  bounds->w=0;
  bounds->h=rowh;
  int x=0;
  int y=0;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--; // Trailing LFs would have us produce ugly blank lines on bottom.
  
  while (srcp<srcc) {
  
    // LF ends the row.
    if (src[srcp]==0x0a) {
      srcp++;
      x=0;
      y+=rowh;
      bounds->h+=rowh;
      continue;
    }
    
    // Other whitespace advances horizontally but does not commit the extra width yet.
    // No vertex is emitted.
    if ((unsigned char)src[srcp]<=0x20) {
      srcp++;
      x+=colw;
      continue;
    }
    
    // Measure the next word. We're naive about this: Words are just one or more non-whitespace chars.
    int tokenc=1;
    while ((srcp+tokenc<srcc)&&((unsigned char)src[srcp+tokenc]>0x20)) tokenc++;
    int wordw=tokenc*colw;
    
    // If the word fits, great. Emit vertices and consume it.
    if (x+wordw<=wlimit) {
      while (tokenc-->0) {
        if (vtxc>=vtxa) goto _done_;
        struct egg_draw_tile *vtx=vtxv+vtxc++;
        vtx->dstx=x+(colw>>1);
        vtx->dsty=y+(rowh>>1);
        vtx->tileid=src[srcp];
        vtx->xform=0;
        srcp++;
        x+=colw;
      }
      if (x>bounds->w) bounds->w=x;
      continue;
    }
    
    // If we're at the start of the row, we have no choice.
    // Break mid-word, emitting as many characters as fit, and always at least one.
    if (!x) {
      tokenc=(wlimit-x)/colw;
      if (tokenc<1) tokenc=1;
      while (tokenc-->0) {
        if (vtxc>=vtxa) goto _done_;
        struct egg_draw_tile *vtx=vtxv+vtxc++;
        vtx->dstx=x+(colw>>1);
        vtx->dsty=y+(rowh>>1);
        vtx->tileid=src[srcp];
        vtx->xform=0;
        srcp++;
        x+=colw;
      }
      if (x>bounds->w) bounds->w=x;
      continue;
    }
    
    // Token exceeds right margin and we're mid-line. Start a new line and don't consume the token.
    y+=rowh;
    bounds->h+=rowh;
    x=0;
  }
 _done_:;
  
  // Add 2 left, 3 right, 1 top, and 1 bottom of padding.
  bounds->w+=5;
  bounds->h+=2;
  bounds->x=(FBW>>1)-(bounds->w>>1);
  bounds->y=FBH-20-bounds->h; // 20 = arbitrary bottom margin
  struct egg_draw_tile *p=vtxv;
  int i=vtxc;
  for (;i-->0;p++) {
    p->dstx+=bounds->x+2;
    p->dsty+=bounds->y+1;
  }
  return vtxc;
}
