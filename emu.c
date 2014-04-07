#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>

#include "link.h"
#include "fifo.h"

#include "vt100.h"
#include "async.h"
#include "main.h"
#include "app.h"
#include "Z80.h"

#include "emu.h"

#include "lang.h"


int watcher = -1;
unsigned char watch_for= 0;
int watch_timeout = -1;
int watch_start;

unsigned int bank = 0;
unsigned int bank_switch = 0, bank_swt_count = 0;

int init_emu_done = 0;
int init_emu_start = 0;

int do_regs = NO;
int do_hard_reset = NO;
int do_reset = NO;


Z80 z80;
byte z80_ram[0x2000];
byte m68k_ram[0x1000000];

/* memory functions {{{ */

byte z80_ram_read(word Addr) {
 return z80_ram[Addr];
}

void z80_ram_write(word Addr, byte Value) {
 z80_ram[Addr] = Value;
}

byte reserved_read(word Addr) {
 return 0;
}

void reserved_write(word Addr, byte Value) {
}

byte chip_read(word Addr) {
 word addr2 = Addr;
 addr2-=0x4000;
 if(addr2<4)
  return YM2612Read(0,Addr);
 return 0;
}

void chip_write(word Addr, byte Value) {
 Addr-=0x4000;
 if(Addr<4) {
  YM2612Write(0,Addr, Value);
 }
}

byte read_68k(word Addr) {
 Addr-=0x8000;
 return m68k_ram[((bank*0x8000)+Addr)%0xffffff];
}

void write_68k(word Addr, byte Value) {
}

void psg_write(word Addr, byte Value) {
 if(Addr == 0x7f11) {
  SN76496Write(0,Value);
 } else {
  if(Addr == 0x6000) {
   bank_switch |= ((Value&1)<<bank_swt_count++);
   if(bank_swt_count == 9) {
    status_dirty = 1;
    bank = bank_switch;
    bank_switch = 0;
    bank_swt_count = 0;
   }
  } else
   reserved_write(Addr, Value);
 }
}

/* }}} */

void *read_proc[] = {
 (void *)z80_ram_read,
 (void *)reserved_read,
 (void *)chip_read,
 (void *)reserved_read,
 (void *)read_68k,
 (void *)read_68k,
 (void *)read_68k,
 (void *)read_68k
};

void *write_proc[] = {
 (void *)z80_ram_write,
 (void *)reserved_write,
 (void *)chip_write,
 (void *)psg_write,
 (void *)write_68k,
 (void *)write_68k,
 (void *)write_68k,
 (void *)write_68k
};


byte InZ80(register word Port) { return 0x00; }
void OutZ80(register word Port, register byte Value) { }
void PatchZ80(register Z80 *R) { }
word LoopZ80(register Z80 *R) { return INT_NONE; }

void WrZ80(word Addr, byte Value) {
 void (*proc)(word Addr, byte Value);
 proc = write_proc[(Addr>>13)%8];
 if(watcher != -1)
  if(Addr == watcher)
   if(Value == watch_for) {
    j_printf("0x%04x changed to 0x%02x!!\n", Addr, Value);
    watcher = -1;
   }
 proc(Addr, Value);
}

byte RdZ80(word Addr) {
 byte (*proc)(word Addr);
 proc = read_proc[(Addr>>13)%8];
 return proc(Addr);
}

void regs(void) {
 const char Flags[9] = "SZ.H.PNC";
 char S[128], T[10];
 int i, j;

 DAsm(S,z80.PC.W);
 for(j=0,i=z80.AF.B.l;j<8;j++,i<<=1) T[j]=i&0x80 ? Flags[j]:'.';
 T[8] = 0;

 j_printf(
  "AF:%04X HL:%04X DE:%04X BC:%04X PC:%04X SP:%04X IX:%04X IY:%04X I:%02X\n",
  z80.AF.W,z80.HL.W,z80.DE.W,z80.BC.W,z80.PC.W,z80.SP.W,z80.IX.W,z80.IY.W,z80.I
 ); 
 j_printf ( 
  "AT PC: [%02X - \33[36m%s\33[0m]   AT SP: [%04X]   FLAGS: [%s]   %s: %s\n",
    RdZ80(z80.PC.W),S,RdZ80(z80.SP.W)+RdZ80(z80.SP.W+1)*256,T,
    z80.IFF&0x04? "IM2":z80.IFF&0x02? "IM1":"IM0",
    z80.IFF&0x01? "EI":"DI"
 );
}



void init_emu(void) {

 for(;;) {
  if(init_emu_start == 1) 
   break;
  SDL_Delay(10);
 }

 ResetZ80(&z80);
 z80.Trace = 1;
 z80.Trap = -1;
 z80.step = NO;
 init_emu_done = 1;
}
