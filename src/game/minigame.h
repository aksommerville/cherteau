/* minigame.h
 * Instances of minigame are exclusively owned by modal_encounter.
 */
 
#ifndef MINIGAME_H
#define MINIGAME_H

struct minigame {
  const char *name;
  
  /* All hooks are required.
   * Owner may continue updating and rendering after the outcome is established.
   */
  void (*del)(struct minigame *minigame); // Frees (minigame).
  void (*update)(struct minigame *minigame,double elapsed,int input,int pvinput);
  void (*render)(struct minigame *minigame); // Always opaque.
  
  int outcome; // -1,0,1 = lose,pending,win
  double difficulty; // 0..1, constant
};

struct minigame *minigame_new_karate(double difficulty);
struct minigame *minigame_new_dance(double difficulty);
struct minigame *minigame_new_jumprope(double difficulty);

#endif
