
/*these are both rated for 60hz */


#define CYCLES_PER_LINE	228
#define NTSC_LINES	262
#define PAL_LINES	313

enum {
 NO, GO
};

extern int watcher;
extern unsigned char watch_for;
extern int watch_timeout;
extern int watch_start;

extern int fm[4];
extern Z80 z80;
extern byte z80_ram[0x2000];
extern byte m68k_ram[0x1000000];

extern unsigned int bank;
extern int init_emu_done;
extern int init_emu_start;
extern int do_regs;
extern int do_hard_reset;
extern int do_reset;

void regs(void);
void init_emu(void);
