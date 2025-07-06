#include "game/game.h"

#define Y_FLOOR 160
#define Y_RANGE 80
#define VELOCITY_LIMIT 5.0 /* sanity limit */
#define WIN_THRESHOLD 42 /* Was 100, but that gets tiresome. */

struct minigame_jumprope {
  struct minigame hdr;
  double rope; // 0..1. 0 is the bottom, 1/2 the top, initial half is upward behind.
  double velocity; // hz
  double veld; // hz/s
  int started;
  int texid;
  int scoreboard_texid;
  int lscore,rscore; // It's kind of silly to have both. They'll always be the same, until one becomes +1 of the other. But it's cute.
  double ljump,rjump; // Jump displacement in pixels. Positive is up.
  double autojump;
  double lpower; // Amount of jump force remaining
  double rtrack; // Rabbit's opinion of the rope's phase.
  double rpower;
  int santa_mode;
  uint8_t prize;
  double xmasclock;
};

#define MINIGAME ((struct minigame_jumprope*)minigame)

/* Delete.
 */
 
static void _jumprope_del(struct minigame *minigame) {
  egg_play_song(RID_song_lock_me_away,0,1);
  egg_texture_del(MINIGAME->texid);
  egg_texture_del(MINIGAME->scoreboard_texid);
  free(minigame);
}

/* Update the lights on the scoreboard.
 * We have decals for both On and Off, so there's no need to redraw the background.
 */
 
static const uint16_t digitbits[11]={
#define DIGIT(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) (a<<15)|(b<<14)|(c<<13)|(d<<12)|(e<<11)|(f<<10)|(g<<9)|(h<<8)|(i<<7)|(j<<6)|(k<<5)|(l<<4)|(m<<3)|(n<<2)|(o<<1),
#define X 1
#define _ 0
  DIGIT( _,X,_, X,_,X, X,_,X, X,_,X, _,X,_) // 0
  DIGIT( _,X,_, X,X,_, _,X,_, _,X,_, X,X,X) // 1
  DIGIT( X,X,_, _,_,X, _,X,_, X,_,_, X,X,X) // 2
  DIGIT( X,X,_, _,_,X, _,X,_, _,_,X, X,X,_) // 3
  DIGIT( X,_,X, X,_,X, X,X,X, _,_,X, _,_,X) // 4
  DIGIT( X,X,X, X,_,_, X,X,_, _,_,X, X,X,_) // 5
  DIGIT( _,X,X, X,_,_, X,X,_, X,_,X, _,X,_) // 6
  DIGIT( X,X,X, _,_,X, _,_,X, _,_,X, _,_,X) // 7
  DIGIT( _,X,_, X,_,X, _,X,_, X,_,X, _,X,_) // 8
  DIGIT( _,X,_, X,_,X, _,X,X, _,_,X, _,_,X) // 9
  DIGIT( _,_,_, X,_,X, _,X,_, X,_,X, _,_,_) // x
#undef DIGIT
#undef X
#undef _
};
 
static void draw_scoreboard_digit(struct minigame *minigame,int dstx0,int dsty,int digit) {
  uint16_t srcbits=0;
  if ((digit>=0)&&(digit<=10)) srcbits=digitbits[digit];
  uint16_t mask=0x8000;
  int yi=5; for (;yi-->0;dsty+=4) {
    int xi=3,dstx=dstx0; for (;xi-->0;dstx+=4,mask>>=1) {
      int srcx=25,srcy=57;
      if (srcbits&mask) srcx=21;
      graf_draw_decal(&g.graf,MINIGAME->texid,dstx,dsty,srcx,srcy,3,3,0);
    }
  }
}
 
static void redraw_scoreboard(struct minigame *minigame) {
  graf_flush(&g.graf);
  graf_set_output(&g.graf,MINIGAME->scoreboard_texid);
  int v;
  if ((v=MINIGAME->lscore)<0) {
    draw_scoreboard_digit(minigame,7,19,10);
    draw_scoreboard_digit(minigame,20,19,10);
  } else {
    if (v>99) v=99;
    draw_scoreboard_digit(minigame,7,19,(v>=10)?(v/10):-1);
    draw_scoreboard_digit(minigame,20,19,v%10);
  }
  if ((v=MINIGAME->rscore)<0) {
    draw_scoreboard_digit(minigame,37,19,10);
    draw_scoreboard_digit(minigame,50,19,10);
  } else {
    if (v>99) v=99;
    draw_scoreboard_digit(minigame,37,19,(v>=10)?(v/10):-1);
    draw_scoreboard_digit(minigame,50,19,v%10);
  }
  graf_flush(&g.graf);
  graf_set_output(&g.graf,1);
}

/* Jump (hero only)
 */
 
static void begin_jump(struct minigame *minigame) {
  if (minigame->outcome) return;
  egg_play_sound(RID_sound_jump);
  MINIGAME->autojump=0.0;
  MINIGAME->ljump=0.001;
  MINIGAME->lpower=150.0;
  MINIGAME->lscore++; // Your score is just your count of jumps.
  redraw_scoreboard(minigame);
}

static void end_jump(struct minigame *minigame) {
  MINIGAME->lpower-=100.0; // don't just zero it; there's a minimum required duration.
  MINIGAME->autojump=0.0;
}

/* Update.
 */
 
static void _jumprope_update(struct minigame *minigame,double elapsed,int input,int pvinput) {
  
  if (!MINIGAME->started) {
    MINIGAME->started=1;
    egg_play_song(RID_song_even_tippier_toe,0,1);
  }
  
  // In Santa mode, it all just runs on a timer.
  // The fancier bits are all in render, we just tick the clock and signal completion.
  if (MINIGAME->santa_mode) {
    MINIGAME->xmasclock+=elapsed;
    if (MINIGAME->xmasclock>=0.5) {
      if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) MINIGAME->xmasclock=5.0;
    }
    if (MINIGAME->xmasclock>=4.0) {
      minigame->outcome=1;
    }
    return;
  }
  
  // The game ends when one score goes negative or exceeds 99.
  if (!minigame->outcome) {
    if (MINIGAME->lscore<0) {
      minigame->outcome=-1;
    } else if (MINIGAME->rscore<0) {
      minigame->outcome=1;
    } else if (MINIGAME->lscore>=WIN_THRESHOLD) {
      minigame->outcome=1;
    } else if (MINIGAME->rscore>=WIN_THRESHOLD) { // shouldn't be possible
      minigame->outcome=-1;
    }
  }
  
  if (!minigame->outcome) {
    if ((MINIGAME->velocity+=MINIGAME->veld*elapsed)>VELOCITY_LIMIT) MINIGAME->velocity=VELOCITY_LIMIT;
    MINIGAME->rope+=MINIGAME->velocity*elapsed;
    if (MINIGAME->rope>=1.0) { // Rope crossed the bottom. Any jumpers here?
      MINIGAME->rope-=1.0;
      if (MINIGAME->ljump<=0.0) {
        if (MINIGAME->rjump<=0.0) { // Both lose. Give it to whoever has more points, otherwise the rabbit.
          if (MINIGAME->lscore>MINIGAME->rscore) MINIGAME->rscore=-1;
          else MINIGAME->lscore=-1;
        } else {
          MINIGAME->lscore=-1;
        }
        redraw_scoreboard(minigame);
      } else if (MINIGAME->rjump<=0.0) {
        MINIGAME->rscore=-1;
        redraw_scoreboard(minigame);
      }
    }
  }
  
  // Hero jumping, gravity, etc.
  if (MINIGAME->lpower>0.0) {
    MINIGAME->lpower-=500.0*elapsed;
    if (MINIGAME->lpower<=0.0) {
      if (MINIGAME->autojump>0.0) {
        begin_jump(minigame);
      }
    } else {
      MINIGAME->ljump+=MINIGAME->lpower*elapsed;
    }
  } else if (MINIGAME->ljump>0.0) {
    MINIGAME->ljump-=120.0*elapsed;
    if (MINIGAME->ljump<=0.0) {
      MINIGAME->ljump=0.0;
    }
  }
  if (MINIGAME->autojump>0.0) MINIGAME->autojump-=elapsed;
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    if (MINIGAME->ljump>0.0) { // Press A while in the air. Allow a brief interval where if we land, we'll start the jump then.
      MINIGAME->autojump=0.200;
    } else {
      begin_jump(minigame);
    }
  } else if (!(input&EGG_BTN_SOUTH)&&(pvinput&EGG_BTN_SOUTH)) {
    end_jump(minigame);
  }
  
  // Rabbit jumping, gravity, etc.
  // Let's not overthink this. He'll jump at a specific rope phase, and his jumps follow the same curve every time.
  // Overthink it just a little tho: Make two jumps per cycle, so when the cycles get too fast, he'll miss the second jump.
  {
    if (MINIGAME->rjump>0.0) {
      MINIGAME->rpower-=600.0*elapsed;
      MINIGAME->rjump+=MINIGAME->rpower*elapsed;
      if (MINIGAME->rjump<=0.0) {
        MINIGAME->rjump=0.0;
      }
    } else if (!minigame->outcome) {
      const double target_phase1=0.900;
      const double target_phase2=0.400;
      if (
        ((MINIGAME->rtrack<=target_phase1)&&(MINIGAME->rope>=target_phase1))||
        ((MINIGAME->rtrack<=target_phase2)&&(MINIGAME->rope>=target_phase2))
      ) {
        egg_play_sound(RID_sound_rabbit_jump);
        MINIGAME->rscore++;
        redraw_scoreboard(minigame);
        MINIGAME->rjump=0.001;
        MINIGAME->rpower=120.0;
      }
      MINIGAME->rtrack=MINIGAME->rope;
    }
  }
}

/* Draw the rope, just a horizontal line, and return its vertical position in fb pixels.
 */
 
static int draw_rope(struct minigame *minigame) {
  double ynorm=(-cos(MINIGAME->rope*M_PI*2.0)+1.0)/2.0;
  int y=Y_FLOOR-(int)(Y_RANGE*ynorm);
  double znorm=(sin(MINIGAME->rope*M_PI*2.0)+1.0)/2.0;
  int luma=(int)(znorm*128.0); // Lighter on the far side. Counterintuitive maybe? But I think the player wants the better contrast at the more dangerous moment.
  if (luma<0) luma=0; else if (luma>0xff) luma=0xff;
  graf_draw_rect(&g.graf,38,y,FBW-76,1,(luma<<24)|(luma<<16)|(luma<<8)|0xff);
  return y;
}

/* Draw Dot or Rabbit.
 * Each must have two frames of equal size. Jumping immediately right of Idle.
 */
 
static void draw_jumper(struct minigame *minigame,int dstx,int srcx,int srcy,int w,int h,double jump) {
  int dsty=Y_FLOOR-h;
  if (jump>0.0) {
    srcx+=w+1;
    dsty-=(int)jump;
  }
  graf_draw_decal(&g.graf,MINIGAME->texid,dstx,dsty,srcx,srcy,w,h,0);
}

/* Santa mode.
 * Draws Dot, Santa, and the prize.
 * Caller should draw background, scoreboard, and the armless refs as usual.
 */
 
static void draw_xmas(struct minigame *minigame) {
  draw_jumper(minigame,100,135,33,35,58,MINIGAME->ljump);
  
  int santax=180;
  int santay=Y_FLOOR-61;
  graf_draw_decal(&g.graf,MINIGAME->texid,santax,santay,143,92,63,61,0);
  if (MINIGAME->xmasclock>3.000) {
    // ok done throwing it, arm disappears.
  } else if (MINIGAME->xmasclock>2.000) {
    // throwing it
    graf_draw_decal(&g.graf,MINIGAME->texid,santax+12,santay+14,207,85,19,18,0);
  } else if (MINIGAME->xmasclock>0.500) {
    // brandishing
    graf_draw_decal(&g.graf,MINIGAME->texid,santax+13,santay+26,207,76,17,8,0);
    graf_draw_tile(&g.graf,MINIGAME->texid,santax+11,santay+28,MINIGAME->prize,0);
  }
  
  // Gift in flight.
  if ((MINIGAME->xmasclock>2.000)&&(MINIGAME->xmasclock<4.000)) {
    const double arch=60.0;
    double y0=santay+28.0;
    double xa=santax+11.0;
    double xz=120.0;
    double t=(MINIGAME->xmasclock-2.000)/2.000;
    if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
    double x=xa*(1.0-t)+xz*t;
    double y=(t-0.5)*2.0;
    y*=y;
    y*=arch;
    y=y0-(arch-y);
    graf_draw_tile(&g.graf,MINIGAME->texid,(int)x,(int)y,MINIGAME->prize,0);
  } else if (MINIGAME->xmasclock>=4.000) {
    graf_draw_tile(&g.graf,MINIGAME->texid,120,santay+28,MINIGAME->prize,0);
  }
}

/* Render.
 */
 
static void _jumprope_render(struct minigame *minigame) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0xc0c0c0ff);
  graf_draw_rect(&g.graf,0,Y_FLOOR,FBW,FBH,0x402010ff);
  
  graf_draw_decal(&g.graf,MINIGAME->scoreboard_texid,(FBW>>1)-(68>>1),10,0,0,68,56,0);
  
  // Draw the rope before the jumpers when it's behind (ie <0.5)
  int ropey;
  if (MINIGAME->santa_mode) {
    draw_xmas(minigame);
  } else if (MINIGAME->rope<0.5) {
    ropey=draw_rope(minigame);
    graf_flush(&g.graf);
    draw_jumper(minigame,100,135,33,35,58,MINIGAME->ljump);
    draw_jumper(minigame,200,207,33,24,42,MINIGAME->rjump);
  } else {
    draw_jumper(minigame,100,135,33,35,58,MINIGAME->ljump);
    draw_jumper(minigame,200,207,33,24,42,MINIGAME->rjump);
    ropey=draw_rope(minigame);
  }
  
  if (!MINIGAME->santa_mode) { // In Santa mode, the refs have no far hand, and there's no rope.
    int yhi=Y_FLOOR-(Y_RANGE>>2);
    int ylo=Y_FLOOR-((Y_RANGE*3)>>2);
    if (ropey<=ylo) { // high rope
      graf_draw_decal(&g.graf,MINIGAME->texid,24,ropey-2,21,41,15,15,0);
      graf_draw_decal(&g.graf,MINIGAME->texid,FBW-24-15,ropey-2,21,41,15,15,EGG_XFORM_XREV);
    } else if (ropey>=yhi) { // low rope
      graf_draw_decal(&g.graf,MINIGAME->texid,24,ropey-12,21,41,15,15,EGG_XFORM_YREV);
      graf_draw_decal(&g.graf,MINIGAME->texid,FBW-24-15,ropey-12,21,41,15,15,EGG_XFORM_YREV|EGG_XFORM_XREV);
    } else { // middle rope
      graf_draw_decal(&g.graf,MINIGAME->texid,22,ropey-2,21,33,18,7,0);
      graf_draw_decal(&g.graf,MINIGAME->texid,FBW-22-18,ropey-2,21,33,18,7,EGG_XFORM_XREV);
    }
  }
  graf_draw_decal(&g.graf,MINIGAME->texid,10,Y_FLOOR-64,1,33,19,65,0);
  graf_draw_decal(&g.graf,MINIGAME->texid,FBW-10-19,Y_FLOOR-64,1,33,19,65,EGG_XFORM_XREV);
}

/* New.
 */
 
static const char *santa_name="santa"; // stable address; we'll use for identification
 
struct minigame *minigame_new_jumprope(double difficulty) {
  struct minigame *minigame=calloc(1,sizeof(struct minigame_jumprope));
  if (!minigame) return 0;
  minigame->name="jumprope";
  minigame->del=_jumprope_del;
  minigame->update=_jumprope_update;
  minigame->render=_jumprope_render;
  minigame->difficulty=(difficulty<=0.0)?0.0:(difficulty>=1.0)?1.0:difficulty;
  
  if (difficulty<0.0) {
    minigame->name=santa_name;
    MINIGAME->santa_mode=1;
    if (g.hp<g.maxhp) MINIGAME->prize=0x00; // heart
    else if (g.gold<999) MINIGAME->prize=0x01; // ten bucks
    else MINIGAME->prize=0x02; // candy cane (noop)
  }
  
  MINIGAME->velocity=0.250;
  MINIGAME->veld=0.010+0.050*difficulty;
  
  if (egg_texture_load_image(MINIGAME->texid=egg_texture_new(),RID_image_jumprope)<0) {
    _jumprope_del(minigame);
    return 0;
  }
  if (egg_texture_load_raw(MINIGAME->scoreboard_texid=egg_texture_new(),68,56,68*4,0,0)<0) {
    _jumprope_del(minigame);
    return 0;
  }
  struct egg_draw_decal decal={
    .dstx=0,.dsty=0,
    .srcx=40,.srcy=79,
    .w=68,.h=56,
    .xform=0,
  };
  egg_draw_decal(MINIGAME->scoreboard_texid,MINIGAME->texid,&decal,1);
  redraw_scoreboard(minigame);
  
  return minigame;
}

/* Get user message in santa mode, and do the thing we talk about.
 */
 
static int is_santa(const struct minigame *minigame) {
  if (!minigame) return 0;
  if (minigame->name==santa_name) return 1;
  return 0;
}
 
int minigame_santa_get_message(char *dst,int dsta,struct minigame *minigame) {
  if (!is_santa(minigame)) return 0;
  switch (MINIGAME->prize) {
    case 0x00: {
        if ((g.hp+=1)>g.maxhp) g.hp=g.maxhp;
        return strings_format(dst,dsta,1,6,0,0);
      }
    case 0x01: {
        if ((g.gold+=10)>999) g.gold=999;
        return strings_format(dst,dsta,1,7,0,0);
      }
    default: return strings_format(dst,dsta,1,8,0,0);
  }
}
