#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>

#include "link.h"

#include "Z80.h"

#include "fifo.h"

#include "vt100.h"
#include "async.h"
#include "main.h"
#include "font.h"
#include "app.h"

#include "pic.xpm"
#include "emu.h"

#include "lang.h"

SDL_Surface *d_screen;
SDL_Surface *d_back;
SDL_Surface *d_pic;

SDL_mutex *out_fifo_mutex;
SDL_mutex *in_fifo_mutex;
SDL_mutex *display_fifo_mutex;


unsigned char font[];

term_t main_term;

int cursor = 0;

int d_dirt = 1;

int prog_done = 0;

typedef struct {
 int r,g,b;
} color_t;


float dest_x, dest_y, dest_w, dest_h;
int floating_x, floating_y;
int floating = 0;
int timer, timer_switch = 0;
int mousemotion = 0;

color_t color_values[] = {
 {0x00, 0x00, 0x00},
 {0xaf, 0x00, 0x00},
 {0x00, 0xaf, 0x00},
 {0xaf, 0xaf, 0x00},

 {0x1f, 0x1f, 0xaf},
 {0xaf, 0x00, 0xaf},
 {0x00, 0xaf, 0xaf},
 {0xaf, 0xaf, 0xaf},

 {0x00, 0x00, 0x00},
 {0xff, 0x00, 0x00},
 {0x00, 0xff, 0x00},
 {0xff, 0xff, 0x00},

 {0x1f, 0x1f, 0xff},
 {0xff, 0x00, 0xff},
 {0x00, 0xff, 0xff},
 {0xff, 0xff, 0xff}
};

Uint32 colors[16];


void init_colors(void) {
 int i;
 for(i=0;i<16;i++) {
  colors[i] = SDL_MapRGB(d_back->format, color_values[i].r, 
                                           color_values[i].g,
			                   color_values[i].b);
 }
}

void d_char(int x, int y, char_t *c) {
 int fg, bg;
 int r,g,b;
 Uint32 color, fg_color, bg_color;
 int real_x, real_y;
 int t,s;
 Uint32 mask;
 union {
  Uint8  *b;
  Uint32 *l;
 } pix, pix2, pix3;

 bg = c->col&0xf;
 fg= (c->col>>4)&0xf;

 /*
 if(bg ==0)

  bg_color = SDL_MapRGB(d_back->format, BACK_SHADE, 
                                        BACK_SHADE, 
					BACK_SHADE);
 else
  bg_color = colors[bg];
*/

 if(fg == 7)
  fg_color = SDL_MapRGB(d_back->format, 0xff, 0xff, 0xff);
 else
  fg_color = colors[fg];

 real_x = x * 8;
 real_y = y * 8;

 pix.b = (Uint8 *)(d_back->pixels+
                   ((real_x)*d_back->format->BytesPerPixel) +
		   ((real_y)*d_back->pitch));

 mask = (d_back->format->Rmask | 
         d_back->format->Gmask |
 	 d_back->format->Bmask |
	 d_back->format->Amask)^~0;

 for(t=0;t<8;t++)
  for(s=0;s<8;s++) {
   if(bg ==0) {
    pix3.b = (unsigned char *)
     (d_pic->pixels+(d_pic->pitch*(real_y+t))+
     (d_pic->format->BytesPerPixel*(real_x+s)));
    r = (*pix3.l & d_pic->format->Rmask)>>d_pic->format->Rshift;
    g = (*pix3.l & d_pic->format->Gmask)>>d_pic->format->Gshift;
    b = (*pix3.l & d_pic->format->Bmask)>>d_pic->format->Bshift;

    bg_color = SDL_MapRGB(d_back->format, r,g,b);

   } else
    bg_color = colors[bg];
   if(((font[ (c->text * 8)+t]>>(8-s)) &1) == 1) 
    color = fg_color; 
   else 
    color = bg_color;
   if(c->text == 0) color = bg_color;
   pix2.b = &pix.b[(t * d_back->pitch)+(s*d_back->format->BytesPerPixel)];
   *pix2.l&=mask;
   *pix2.l|=color;
  }


}

void d_drawcursor(void) {
 int real_x, real_y;
 int t,s;
 Uint32 mask;
 Uint32 color;
 union {
  Uint8  *b;
  Uint32 *l;
 } pix, pix2;

 if(cursor == 1) { 


  real_x = main_term.cursor_x * 8;
  real_y = main_term.cursor_y * 8;

  pix.b = (Uint8 *)(d_back->pixels+
                   ((real_x)*d_back->format->BytesPerPixel) +
		   ((real_y)*d_back->pitch));

  mask = (d_back->format->Rmask | 
          d_back->format->Gmask |
       	  d_back->format->Bmask |
	  d_back->format->Amask)^~0;

  color = SDL_MapRGB(d_back->format, 0xff, 0xff, 0xff);

  for(t=0;t<8;t++)
   for(s=0;s<8;s++) {
    pix2.b = &pix.b[(t * d_back->pitch)+(s*d_back->format->BytesPerPixel)];
    *pix2.l&=mask;
    *pix2.l|=color;
   }
 } else
  d_char(main_term.cursor_x, main_term.cursor_y, 
         &main_term.chars[main_term.cursor_x+(WIDTH*main_term.cursor_y)]);
}
void d_drawscreen(void) {
 int x, y; 
 float r,g,b;
 int rp, gp, bp;
 int q;
 union {
  unsigned char *b;
  unsigned int  *l;
 } back_pix, ovr_pix, screen_pix;

 if(main_term.dirty == 1) {
  for(y=0;y<HEIGHT;y++)
   for(x=0;x<WIDTH;x++) 
    d_char(x,y, &main_term.chars[x+(WIDTH*y)]);
   
  main_term.dirty = 0;
 }
 d_drawcursor();

 SDL_BlitSurface(d_back, NULL, d_screen, NULL);
 SDL_UpdateRect(d_screen, 0,0,0,0);
}

void q_input(void) {
 SDL_Event event;
 struct winsize wsize;

 while(SDL_PollEvent(&event)) { 
  switch(event.type) {
   case SDL_QUIT:
    SDL_Quit();
    exit(0);
    break;
   case SDL_KEYDOWN:
    if((SDL_GetModState() & KMOD_LALT)==KMOD_LALT) {
     if(floating == 0) {
      floating_x = main_term.cursor_x;
      floating_y = main_term.cursor_y;
      floating = 1;
     }
     switch(event.key.keysym.sym) {
     /* {{{ */
      case SDLK_UP:
       timer_switch = 1;
       timer = 0;
       dest_w = BOX_W;
       dest_h = BOX_H;
       floating_y--;
       if(floating_y<0)
	floating_y = 0;
       dest_x = (floating_x *8) - (BOX_W/2);
       dest_y = (floating_y *8) - (BOX_H/2);
       mousemotion = 1;
       break;
      case SDLK_DOWN:
       timer_switch = 1;
       timer = 0;
       dest_w = BOX_W;
       dest_h = BOX_H;
       floating_y++;
       if(floating_y>=HEIGHT)
	floating_y = HEIGHT-1;
       dest_x = (floating_x *8) - (BOX_W/2);
       dest_y = (floating_y *8) - (BOX_H/2);
       mousemotion = 1;
       break;
      case SDLK_LEFT:
       timer_switch = 1;
       timer = 0;
       dest_w = BOX_W;
       dest_h = BOX_H;
       floating_x--;
       if(floating_y<0)
	floating_x = 0;
       dest_x = (floating_x *8) - (BOX_W/2);
       dest_y = (floating_y *8) - (BOX_H/2);
       mousemotion = 1;
       break;
      case SDLK_RIGHT:
       timer_switch = 1;
       timer = 0;
       dest_w = BOX_W;
       dest_h = BOX_H;
       floating_x++;
       if(floating_x>=WIDTH)
	floating_x = WIDTH-1;
       dest_x = (floating_x *8) - (BOX_W/2);
       dest_y = (floating_y *8) - (BOX_H/2);
       mousemotion = 1;
       break;
       /* }}} */
     }
    } else {
     floating = 0;
    switch(event.key.keysym.sym) {
     /* {{{ */
     case SDLK_ESCAPE:
      main_term.char_out('\33', (struct term_t *)&main_term);
      break;
     case SDLK_RETURN:
      main_term.char_out('\r', (struct term_t *)&main_term);
      break;
     case SDLK_LEFT:
      vt_send((struct term_t *)&main_term, K_LT);
      break;
     case SDLK_RIGHT:
       vt_send((struct term_t *)&main_term, K_RT);
       break;
     case SDLK_UP:
      vt_send((struct term_t *)&main_term, K_UP);
      break;
     case SDLK_DOWN: 
      vt_send((struct term_t *)&main_term, K_DN);
      break;
     case SDLK_PAGEUP:
      vt_send((struct term_t *)&main_term, K_PGUP);
      break;
     case SDLK_PAGEDOWN:
      vt_send((struct term_t *)&main_term, K_PGDN);
      break;
     case SDLK_HOME:
      vt_send((struct term_t *)&main_term, K_HOME);
      break;
     case SDLK_INSERT:
      vt_send((struct term_t *)&main_term, K_INS);
      break;
     case SDLK_DELETE:
      vt_send((struct term_t *)&main_term, K_DEL);
      break;
     case SDLK_END:
      vt_send((struct term_t *)&main_term, K_END);
      break;
     case SDLK_TAB:
      main_term.char_out(main_term.control_keys[CONTROL_TAB],
       (struct term_t *)&main_term);
      break;
     case SDLK_BACKSPACE:
      main_term.char_out(main_term.control_keys[CONTROL_BACKSPACE],
       (struct term_t *)&main_term);
      break;
     case SDLK_NUMLOCK:
     case SDLK_CAPSLOCK:
     case SDLK_SCROLLOCK:
     case SDLK_RSHIFT:
     case SDLK_LSHIFT:
     case SDLK_RCTRL:
     case SDLK_LCTRL:
     case SDLK_RALT:
     case SDLK_LALT:
     case SDLK_RMETA:
     case SDLK_LMETA:
     case SDLK_LSUPER:
     case SDLK_RSUPER:
     case SDLK_MODE:
     case SDLK_COMPOSE:
      break;
     default:
      main_term.char_out(event.key.keysym.unicode, (struct term_t *)&main_term);
     break; 
     /* }}} */
    }

  }
 }
 }
}

int d_write(unsigned char *buf, int l) {
 int i;
 for(i=0;i<l;i++) 
  write_char_fifo(&out_fifo, buf[i]);

 return i;
}

void d_putc(int c) {
 SDL_mutexP(out_fifo_mutex);
 if(c == '\n') 
  write_char_fifo(&out_fifo, '\r');
 write_char_fifo(&out_fifo, c);
 SDL_mutexV(out_fifo_mutex);
}

void d_puts(char *in) {
 int i;
 for(i=0;;i++) {
  if(in[i] == 0) break;
  d_putc(in[i]);
 }
 d_putc('\n');
}

void d_put(char *in) {
 int i;
 for(i=0;;i++) {
  if(in[i] == 0) break;
  d_putc(in[i]);
 }
}

void d_printf(char *fmt, ...) {
 int i;
 char buf[PRINTF_BUF_LEN]; 
 va_list ap;
 va_start(ap, fmt);
 vsnprintf(buf, PRINTF_BUF_LEN, fmt, ap);
 for(i=0;;i++) {
  if(buf[i] == 0) break;
  d_putc(buf[i]);
 }
 va_end(ap);
}

void j_putc(int c) {
 SDL_mutexP(display_fifo_mutex);
 write_char_fifo(&display_fifo, c);
 SDL_mutexV(display_fifo_mutex);
}

void j_puts(char *in) {
 int i;
 for(i=0;;i++) {
  if(in[i] == 0) break;
  j_putc(in[i]);
 }
 j_putc('\n');
}

void j_put(char *in) {
 int i;
 for(i=0;;i++) {
  if(in[i] == 0) break;
  j_putc(in[i]);
 }
}

void j_printf(char *fmt, ...) {
 int i;
 char buf[PRINTF_BUF_LEN]; 
 va_list ap;
 va_start(ap, fmt);
 vsnprintf(buf, PRINTF_BUF_LEN, fmt, ap);
 for(i=0;;i++) {
  if(buf[i] == 0) break;
  j_putc(buf[i]);
 }
 va_end(ap);
}



void main_tick(void) {
 int lcursor_x, lcursor_y;
 float fx,fy,fw,fh;
 d_drawscreen();
 q_input();
 if((main_term.cursor_x != lcursor_x) |
    (main_term.cursor_y != lcursor_y)) {
  timer_switch = 1;
  timer = 0;
  mousemotion = 0;
 }

 lcursor_x = main_term.cursor_x;
 lcursor_y = main_term.cursor_y;

 if(timer_switch == 1) {
  timer++;
  if(timer == MAX_TIMER)
   timer_switch = 0;
 }
 if(timer < (MAX_TIMER/2)) {
  if(mousemotion == 0) {
   dest_w = BOX_W; 
   dest_h = BOX_H;
   dest_x = (main_term.cursor_x *SPEED) - (BOX_W/2);
   dest_y = (main_term.cursor_y *SPEED) - (BOX_H/2);
  }
 } else {
  dest_x = 0;
  dest_y = 0;
  dest_w = d_screen->w;
  dest_h = d_screen->h;
  mousemotion = 0;
 }
}

void io_tick(void) {
 int j, c;
  for(j=0;j<(WIDTH * HEIGHT);j++) {
   SDL_mutexP(out_fifo_mutex);
   c = read_char_fifo(&out_fifo);
   SDL_mutexV(out_fifo_mutex);
   if(c==-1) break;
   vt_out((struct term_t *)&main_term, c);
  }  
}

void do_tick(void) {
 if(do_regs == GO) {
  regs();
  do_regs = NO;
 }
 if(do_hard_reset == GO) {
  ResetZ80(&z80);
  do_hard_reset = NO; 
 }
 if(do_reset == GO) {
  IntZ80(&z80, INT_NMI);
  do_reset = NO;
 }
}


int debug_thread(void *a) {
 for(;;) {
  if(z80.Trace == 1) {
   j_puts("\33[0mz80 emulation stopped");
   status_dirty = 1;
   for(;;) {
    do_tick();
    SDL_Delay(10);
    if(z80.Trace == 0) {
     j_puts("\33[0mz80 emulation started");
     status_dirty = 1;
     break;
    }
    if(z80.step == GO) {
     regs();
     z80.Trace = 0;
     ExecZ80(&z80, 1);
     z80.Trace = 1;
     z80.step = NO;
    }
    if(prog_done == 1) exit(0);
   }
  }
  SDL_Delay(10);
 }
}

int main(int argc, char **argv) {
 int lcursor_x, lcursor_y;
 float fx, fy, fw, fh;
 int i, j, line;
 int c;
 int pause_trip = 0;

 Uint32 ticks;
 SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
 if((d_screen = SDL_SetVideoMode(WIDTH*8, HEIGHT * 8, 24, 0 ))<=0) {
  fprintf(stderr, "%s\n", SDL_GetError());
  exit(0);
 }

 SDL_WM_SetCaption("z80", "z80");
 
 d_back = SDL_CreateRGBSurface(0, WIDTH*8,HEIGHT*8,24,
  d_screen->format->Rmask,
  d_screen->format->Gmask,
  d_screen->format->Bmask,
  d_screen->format->Amask);

 init_colors();

 SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 
                     SDL_DEFAULT_REPEAT_INTERVAL);
 SDL_EnableUNICODE(1);

 init_term((struct term_t *)&main_term, callback, WIDTH, HEIGHT);
 main_term.control_keys[CONTROL_BACKSPACE] = BACKSPACE;
 main_term.control_keys[CONTROL_QUIT] = QUIT;
 main_term.control_keys[CONTROL_DOWN] = 0xa;
 main_term.control_keys[CONTROL_TAB] = TAB;
 main_term.control_keys[CONTROL_AAA] = 1;

 d_pic = (SDL_Surface *)IMG_ReadXPMFromArray(pic_xpm);

 SDL_SetTimer(400, sig_alarm);
 lcursor_x = main_term.cursor_x;
 lcursor_y = main_term.cursor_y;
 fx = 0;
 fy = 0;
 fw = 10;
 fh = 10;
 dest_x = 0;
 dest_y = 0;
 dest_w = d_screen->w;
 dest_h = d_screen->h;
 timer = 0;
 timer_switch = 0;
 out_fifo_mutex = SDL_CreateMutex();
 in_fifo_mutex = SDL_CreateMutex();
 display_fifo_mutex= SDL_CreateMutex();
 SDL_CreateThread(app, NULL);
 init_emu();
 sound_init();
 SDL_CreateThread(debug_thread, NULL);
 for(i=0;;i++) {

  io_tick(); 
  main_tick();
  SDL_Delay(10);

  if(watch_timeout!=-1)
   if((SDL_GetTicks()-watch_timeout)>=watch_start) {
    watch_timeout=-1;
    j_puts("watch timeout");
   }

  if(prog_done == 1) break;
  if(pause_trip ==0)
   if(i==5) {
    SDL_PauseAudio(0);
    pause_trip = 1;
   }
 }
 return 0;
}

