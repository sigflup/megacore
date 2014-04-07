/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>

#include "string.h"

#include "snd.h"
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


unsigned int sound_on = 1;      /* sound enabled */
unsigned int sound_psg = 1;     /* psg enabled */
unsigned int sound_fm = 1;      /* fm enabled */
unsigned int sound_filter = 50; /* low-pass filter percentage (0-100) */

uint16 sound_soundbuf[2][SOUND_MAXRATE];

int sound_init(void)
{

  soundp_start();

  YM2612Init(1, NTSC_CLOCK / 7, SOUND_SAMPLERATE, NULL, NULL);
  SN76496Init(0, NTSC_CLOCK / 15, 0, SOUND_SAMPLERATE);
}


void sound_process(int num)
{
  static sint16 *tbuf[2];
  uint16 sn76496buf[SOUND_MAXRATE];        /* far too much but who cares */
  unsigned int samples = num;
  unsigned int i;
  static sint32 ll, rr;
  uint32 factora = (0x10000 * sound_filter) / 100;
  uint32 factorb = 0x10000 - factora;

  tbuf[0] = sound_soundbuf[0];
  tbuf[1] = sound_soundbuf[1];

  if (sound_fm)
   YM2612UpdateOne(0, tbuf, samples);

    /* SN76496 outputs sound in range -0x4000 to 0x3fff
       YM2612 ouputs sound in range -0x8000 to 0x7fff - therefore
       we take 3/4 of the YM2612 and add half the SN76496 */

    /* this uses a single-pole low-pass filter (6 dB/octave) curtesy of
       Jon Watte <hplus@mindcontrol.org> */

 if (sound_fm && sound_psg) {
  SN76496Update(0, sn76496buf, samples);
   for (i = 0; i < samples; i++) {
     sint16 snsample = ((sint16)sn76496buf[i] - 0x4000) >> 1;
     sint32 l = snsample + ((tbuf[0][i] * 3) >> 2); /* left channel */
     sint32 r = snsample + ((tbuf[1][i] * 3) >> 2); /* right channel */
     ll = (ll >> 16) * factora + (l * factorb);
     rr = (rr >> 16) * factora + (r * factorb);
     tbuf[0][i] = ll >> 16;
     tbuf[1][i] = rr >> 16;
   }
 } else if (!sound_fm && !sound_psg) {
   /* no sound */
   memset(tbuf[0], 0, 2 * samples);
   memset(tbuf[1], 0, 2 * samples);
 } else if (sound_fm) {
   /* fm only */
   for (i = 0; i < samples; i++) {
     sint32 l = (tbuf[0][i] * 3) >> 2; /* left channel */
     sint32 r = (tbuf[1][i] * 3) >> 2; /* right channel */
     ll = (ll >> 16) * factora + (l * factorb);
     rr = (rr >> 16) * factora + (r * factorb);
     tbuf[0][i] = ll >> 16;
     tbuf[1][i] = rr >> 16;
   }
 } else {
   /* psg only */
   SN76496Update(0, sn76496buf, samples);
   for (i = 0; i < samples; i++) {
     sint16 snsample = ((sint16)(sn76496buf[i] - 0x4000)) >> 1;
     ll = (ll >> 16) * factora + (snsample * factorb);
     rr = (rr >> 16) * factora + (snsample * factorb);
     tbuf[0][i] = ll >> 16;
     tbuf[1][i] = rr >> 16;
   }
  }
}


