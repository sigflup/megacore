
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>

#include "string.h"

#include "snd.h"
#include "sndp.h"
#include "sn76496.h"

#include "support.h"
#include "fm.h"


#include "link.h"
#include "fifo.h"

#include "vt100.h"
#include "async.h"
#include "main.h"
#include "app.h"
#include "Z80.h"

#include "emu.h"

#include "lang.h"

#define MAX_BUFFER	16384

SDL_mutex *sound_fifo_mutex;
char_fifo_t sound_fifo = { (char *)0, 0,0 };

SDL_AudioSpec *spec;

/*** soundp_start - start sound hardware ***/

int cycle = 0;
int wait_state = 0;
float add;

int dacout_index;
int dacout[SOUND_MAXRATE];
int dacout_head;

void audio_callback(void *userdata, Uint8 *stream, int len) {
 Uint16 buffer[SOUND_MAXRATE];
 int i;
 Uint8 *s; 
 float add;

 do_tick();
 add = (((float)len/4) * (1.0f/(float)spec->freq));
 if(ntsc_pal == NTSC)
  add*= (CYCLES_PER_LINE * NTSC_LINES * 60);
 else
  add*= (CYCLES_PER_LINE * PAL_LINES * 50);;
 dacout_index = 0; 
 ExecZ80(&z80,(int)add);
 cycle+=add;
 if(cycle >= (ntsc_pal == NTSC ? 71364 : 59736 )) {
  IntZ80(&z80, 0x38);
  cycle = 0;
  if(wait_state == 2)
   wait_state = 0;
  if(wait_state == 1)
   wait_state = 2;
  
 }


 dacout_index = 0;
 sound_process(len/4);

 for(i=0;i<len/4;i++) {
  buffer[i*2]=  sound_soundbuf[0][i];
  buffer[(i*2)+1]=sound_soundbuf[1][i];
 }

 s = (Uint8 *)&buffer[0];
 for(i=0;i<len;i++) {
  stream[i] = s[i];
 }

 

}

int soundp_start(void)
{
 SDL_AudioSpec des;
 SDL_AudioSpec *got;
 des.freq = SOUND_MAXRATE;
 des.format = AUDIO_S16LSB;
 des.channels=2;
 des.samples=1024;
 des.callback=audio_callback;
 got = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
 if(SDL_OpenAudio(&des, got)<0) {
  fprintf(stderr, "Couldn't open audio: %s", SDL_GetError());
  exit(-1);
 } 
 spec = got;

 sound_fifo_mutex = SDL_CreateMutex();

}
