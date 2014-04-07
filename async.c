
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "fifo.h"

#include "vt100.h"
#include "async.h"
#include "main.h"

char_fifo_t in_fifo = { (char *)0, 0,0 };
char_fifo_t out_fifo= { (char *)0, 0,0 };

char_fifo_t display_fifo = { (char *)0, 0,0};

Uint32 sig_alarm(Uint32 interval) {
 cursor^=1;
 return interval;
}

void callback(char c, struct term_t *win) {
 cursor = 1;
 SDL_mutexP(in_fifo_mutex);
 write_char_fifo(&in_fifo, c);
 SDL_mutexV(in_fifo_mutex);
}

/* this should only be called by your app thread. It blocks */

int d_getc(void) {
 int c; 
 SDL_mutexP(in_fifo_mutex);
 c = read_char_fifo(&in_fifo);
 SDL_mutexV(in_fifo_mutex);
 return c; 
}

