#include "support.h"
#include "fm.h"

#define LOG_OFF	0
#define GYM	1
#define GNM	2



#define NTSC_CLOCK  53693100
#define PAL_CLOCK  53203424

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int sint32;


int soundp_start(void);
void soundp_stop(void);
int soundp_samplesbufferd(void);
void soundp_output(uint16 *left, uint16 *righ, unsigned int samples);
void ui_err(char *message);

#define SOUND_MAXRATE 44100 
#define SOUND_SAMPLERATE 44100 

extern int vdp_framerate;
extern int vdp_clock;
extern int gen_musiclog;
extern unsigned int vdp_totlines;

extern int sound_debug;
extern int sound_feedback;
extern unsigned int sound_minfields;
extern unsigned int sound_maxfields;
extern unsigned int sound_speed;
extern unsigned int sound_sampsperfield;
extern unsigned int sound_threshold;
extern uint8 sound_regs1[256];
extern uint8 sound_regs2[256];
extern uint8 sound_address1;
extern uint8 sound_address2;
extern uint8 sound_keys[8];
extern unsigned int sound_on;
extern unsigned int sound_psg;
extern unsigned int sound_fm;
extern uint16 sound_soundbuf[2][SOUND_MAXRATE];
extern unsigned int sound_filter;

void frame_samples(void);
int sound_start(void);
void sound_stop(void);
int sound_init(void);
void sound_final(void);
int sound_reset(void);
void sound_startfield(void);
void sound_endfield(void);
void sound_genreset(void);
uint8 sound_ym2612fetch(uint8 addr);
void sound_ym2612store(uint8 addr, uint8 data);
void sound_sn76496store(uint8 data);
void sound_genreset(void);
void sound_process(int num);
void sound_line(void);
