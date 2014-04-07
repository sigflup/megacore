/** Z80: portable Z80 emulator *******************************/
/**                                                         **/
/**                         Debug.c                         **/
/**                                                         **/
/** This file contains the built-in debugging routine for   **/
/** the Z80 emulator which is called on each Z80 step when  **/
/** Trap!=0.                                                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1995-2007                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifdef DEBUG



#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <SDL.h>

#include "Z80.h"
#include "Debug.h"

#ifdef FMSX
#include "AY8910.h"
extern AY8910 PSG;
#endif


const char *Mnemonics[256] =
{
  "NOP","LD BC,#","LD (BC),A","INC BC","INC B","DEC B","LD B,*","RLCA",
  "EX AF,AF'","ADD HL,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*","RRCA",
  "DJNZ @","LD DE,#","LD (DE),A","INC DE","INC D","DEC D","LD D,*","RLA",
  "JR @","ADD HL,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*","RRA",
  "JR NZ,@","LD HL,#","LD (#),HL","INC HL","INC H","DEC H","LD H,*","DAA",
  "JR Z,@","ADD HL,HL","LD HL,(#)","DEC HL","INC L","DEC L","LD L,*","CPL",
  "JR NC,@","LD SP,#","LD (#),A","INC SP","INC (HL)","DEC (HL)","LD (HL),*","SCF",
  "JR C,@","ADD HL,SP","LD A,(#)","DEC SP","INC A","DEC A","LD A,*","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,H","LD B,L","LD B,(HL)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,H","LD C,L","LD C,(HL)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,H","LD D,L","LD D,(HL)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,H","LD E,L","LD E,(HL)","LD E,A",
  "LD H,B","LD H,C","LD H,D","LD H,E","LD H,H","LD H,L","LD H,(HL)","LD H,A",
  "LD L,B","LD L,C","LD L,D","LD L,E","LD L,H","LD L,L","LD L,(HL)","LD L,A",
  "LD (HL),B","LD (HL),C","LD (HL),D","LD (HL),E","LD (HL),H","LD (HL),L","HALT","LD (HL),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,H","LD A,L","LD A,(HL)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD H","ADD L","ADD (HL)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC H","ADC L","ADC (HL)","ADC A",
  "SUB B","SUB C","SUB D","SUB E","SUB H","SUB L","SUB (HL)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC H","SBC L","SBC (HL)","SBC A",
  "AND B","AND C","AND D","AND E","AND H","AND L","AND (HL)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR H","XOR L","XOR (HL)","XOR A",
  "OR B","OR C","OR D","OR E","OR H","OR L","OR (HL)","OR A",
  "CP B","CP C","CP D","CP E","CP H","CP L","CP (HL)","CP A",
  "RET NZ","POP BC","JP NZ,#","JP #","CALL NZ,#","PUSH BC","ADD *","RST 0x00",
  "RET Z","RET","JP Z,#","PFX_CB","CALL Z,#","CALL #","ADC *","RST 0x08",
  "RET NC","POP DE","JP NC,#","OUTA (*)","CALL NC,#","PUSH DE","SUB *","RST 0x10",
  "RET C","EXX","JP C,#","INA (*)","CALL C,#","PFX_DD","SBC *","RST 0x18",
  "RET PO","POP HL","JP PO,#","EX HL,(SP)","CALL PO,#","PUSH HL","AND *","RST 0x20",
  "RET PE","LD PC,HL","JP PE,#","EX DE,HL","CALL PE,#","PFX_ED","XOR *","RST 0x28",
  "RET P","POP AF","JP P,#","DI","CALL P,#","PUSH AF","OR *","RST 0x30",
  "RET M","LD SP,HL","JP M,#","EI","CALL M,#","PFX_FD","CP *","RST 0x38"
};

const char *MnemonicsCB[256] =
{
  "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC (HL)","RLC A",
  "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC (HL)","RRC A",
  "RL B","RL C","RL D","RL E","RL H","RL L","RL (HL)","RL A",
  "RR B","RR C","RR D","RR E","RR H","RR L","RR (HL)","RR A",
  "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA (HL)","SLA A",
  "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA (HL)","SRA A",
  "SLL B","SLL C","SLL D","SLL E","SLL H","SLL L","SLL (HL)","SLL A",
  "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL (HL)","SRL A",
  "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(HL)","BIT 0,A",
  "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(HL)","BIT 1,A",
  "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(HL)","BIT 2,A",
  "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(HL)","BIT 3,A",
  "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(HL)","BIT 4,A",
  "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(HL)","BIT 5,A",
  "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(HL)","BIT 6,A",
  "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(HL)","BIT 7,A",
  "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(HL)","RES 0,A",
  "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(HL)","RES 1,A",
  "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(HL)","RES 2,A",
  "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(HL)","RES 3,A",
  "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(HL)","RES 4,A",
  "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(HL)","RES 5,A",
  "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(HL)","RES 6,A",
  "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(HL)","RES 7,A",
  "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(HL)","SET 0,A",
  "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(HL)","SET 1,A",
  "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(HL)","SET 2,A",
  "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(HL)","SET 3,A",
  "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(HL)","SET 4,A",
  "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(HL)","SET 5,A",
  "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(HL)","SET 6,A",
  "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(HL)","SET 7,A"
};

const char *MnemonicsED[256] =
{
  "DB 0xED,0x00","DB 0xED,0x01","DB 0xED,0x02","DB 0xED,0x03",
  "DB 0xED,0x04","DB 0xED,0x05","DB 0xED,0x06","DB 0xED,0x07",
  "DB 0xED,0x08","DB 0xED,0x09","DB 0xED,0x0A","DB 0xED,0x0B",
  "DB 0xED,0x0C","DB 0xED,0x0D","DB 0xED,0x0E","DB 0xED,0x0F",
  "DB 0xED,0x10","DB 0xED,0x11","DB 0xED,0x12","DB 0xED,0x13",
  "DB 0xED,0x14","DB 0xED,0x15","DB 0xED,0x16","DB 0xED,0x17",
  "DB 0xED,0x18","DB 0xED,0x19","DB 0xED,0x1A","DB 0xED,0x1B",
  "DB 0xED,0x1C","DB 0xED,0x1D","DB 0xED,0x1E","DB 0xED,0x1F",
  "DB 0xED,0x20","DB 0xED,0x21","DB 0xED,0x22","DB 0xED,0x23",
  "DB 0xED,0x24","DB 0xED,0x25","DB 0xED,0x26","DB 0xED,0x27",
  "DB 0xED,0x28","DB 0xED,0x29","DB 0xED,0x2A","DB 0xED,0x2B",
  "DB 0xED,0x2C","DB 0xED,0x2D","DB 0xED,0x2E","DB 0xED,0x2F",
  "DB 0xED,0x30","DB 0xED,0x31","DB 0xED,0x32","DB 0xED,0x33",
  "DB 0xED,0x34","DB 0xED,0x35","DB 0xED,0x36","DB 0xED,0x37",
  "DB 0xED,0x38","DB 0xED,0x39","DB 0xED,0x3A","DB 0xED,0x3B",
  "DB 0xED,0x3C","DB 0xED,0x3D","DB 0xED,0x3E","DB 0xED,0x3F",
  "IN B,(C)","OUT (C),B","SBC HL,BC","LD (#),BC",
  "NEG","RETN","IM 0","LD I,A",
  "IN C,(C)","OUT (C),C","ADC HL,BC","LD BC,(#)",
  "DB 0xED,0x4C","RETI","DB 0xED,0x4E","LD R,A",
  "IN D,(C)","OUT (C),D","SBC HL,DE","LD (#),DE",
  "DB 0xED,0x54","DB 0xED,0x55","IM 1","LD A,I",
  "IN E,(C)","OUT (C),E","ADC HL,DE","LD DE,(#)",
  "DB 0xED,0x5C","DB 0xED,0x5D","IM 2","LD A,R",
  "IN H,(C)","OUT (C),H","SBC HL,HL","LD (#),HL",
  "DB 0xED,0x64","DB 0xED,0x65","DB 0xED,0x66","RRD",
  "IN L,(C)","OUT (C),L","ADC HL,HL","LD HL,(#)",
  "DB 0xED,0x6C","DB 0xED,0x6D","DB 0xED,0x6E","RLD",
  "IN F,(C)","DB 0xED,0x71","SBC HL,SP","LD (#),SP",
  "DB 0xED,0x74","DB 0xED,0x75","DB 0xED,0x76","DB 0xED,0x77",
  "IN A,(C)","OUT (C),A","ADC HL,SP","LD SP,(#)",
  "DB 0xED,0x7C","DB 0xED,0x7D","DB 0xED,0x7E","DB 0xED,0x7F",
  "DB 0xED,0x80","DB 0xED,0x81","DB 0xED,0x82","DB 0xED,0x83",
  "DB 0xED,0x84","DB 0xED,0x85","DB 0xED,0x86","DB 0xED,0x87",
  "DB 0xED,0x88","DB 0xED,0x89","DB 0xED,0x8A","DB 0xED,0x8B",
  "DB 0xED,0x8C","DB 0xED,0x8D","DB 0xED,0x8E","DB 0xED,0x8F",
  "DB 0xED,0x90","DB 0xED,0x91","DB 0xED,0x92","DB 0xED,0x93",
  "DB 0xED,0x94","DB 0xED,0x95","DB 0xED,0x96","DB 0xED,0x97",
  "DB 0xED,0x98","DB 0xED,0x99","DB 0xED,0x9A","DB 0xED,0x9B",
  "DB 0xED,0x9C","DB 0xED,0x9D","DB 0xED,0x9E","DB 0xED,0x9F",
  "LDI","CPI","INI","OUTI",
  "DB 0xED,0xA4","DB 0xED,0xA5","DB 0xED,0xA6","DB 0xED,0xA7",
  "LDD","CPD","IND","OUTD",
  "DB 0xED,0xAC","DB 0xED,0xAD","DB 0xED,0xAE","DB 0xED,0xAF",
  "LDIR","CPIR","INIR","OTIR",
  "DB 0xED,0xB4","DB 0xED,0xB5","DB 0xED,0xB6","DB 0xED,0xB7",
  "LDDR","CPDR","INDR","OTDR",
  "DB 0xED,0xBC","DB 0xED,0xBD","DB 0xED,0xBE","DB 0xED,0xBF",
  "DB 0xED,0xC0","DB 0xED,0xC1","DB 0xED,0xC2","DB 0xED,0xC3",
  "DB 0xED,0xC4","DB 0xED,0xC5","DB 0xED,0xC6","DB 0xED,0xC7",
  "DB 0xED,0xC8","DB 0xED,0xC9","DB 0xED,0xCA","DB 0xED,0xCB",
  "DB 0xED,0xCC","DB 0xED,0xCD","DB 0xED,0xCE","DB 0xED,0xCF",
  "DB 0xED,0xD0","DB 0xED,0xD1","DB 0xED,0xD2","DB 0xED,0xD3",
  "DB 0xED,0xD4","DB 0xED,0xD5","DB 0xED,0xD6","DB 0xED,0xD7",
  "DB 0xED,0xD8","DB 0xED,0xD9","DB 0xED,0xDA","DB 0xED,0xDB",
  "DB 0xED,0xDC","DB 0xED,0xDD","DB 0xED,0xDE","DB 0xED,0xDF",
  "DB 0xED,0xE0","DB 0xED,0xE1","DB 0xED,0xE2","DB 0xED,0xE3",
  "DB 0xED,0xE4","DB 0xED,0xE5","DB 0xED,0xE6","DB 0xED,0xE7",
  "DB 0xED,0xE8","DB 0xED,0xE9","DB 0xED,0xEA","DB 0xED,0xEB",
  "DB 0xED,0xEC","DB 0xED,0xED","DB 0xED,0xEE","DB 0xED,0xEF",
  "DB 0xED,0xF0","DB 0xED,0xF1","DB 0xED,0xF2","DB 0xED,0xF3",
  "DB 0xED,0xF4","DB 0xED,0xF5","DB 0xED,0xF6","DB 0xED,0xF7",
  "DB 0xED,0xF8","DB 0xED,0xF9","DB 0xED,0xFA","DB 0xED,0xFB",
  "DB 0xED,0xFC","DB 0xED,0xFD","DB 0xED,0xFE","DB 0xED,0xFF"
};

const char *MnemonicsXX[256] =
{
  "NOP","LD BC,#","LD (BC),A","INC BC","INC B","DEC B","LD B,*","RLCA",
  "EX AF,AF'","ADD I%,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*","RRCA",
  "DJNZ @","LD DE,#","LD (DE),A","INC DE","INC D","DEC D","LD D,*","RLA",
  "JR @","ADD I%,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*","RRA",
  "JR NZ,@","LD I%,#","LD (#),I%","INC I%","INC I%","DEC I%","LD I%,*","DAA",
  "JR Z,@","ADD I%,I%","LD I%,(#)","DEC I%","INC I%l","DEC I%l","LD I%l,*","CPL",
  "JR NC,@","LD SP,#","LD (#),A","INC SP","INC (I%+^)","DEC (I%+^)","LD (I%+^),*","SCF",
  "JR C,@","ADD I%,SP","LD A,(#)","DEC SP","INC A","DEC A","LD A,*","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,I%","LD B,I%l","LD B,(I%+^)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,I%","LD C,I%l","LD C,(I%+^)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,I%","LD D,I%l","LD D,(I%+^)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,I%","LD E,I%l","LD E,(I%+^)","LD E,A",
  "LD I%,B","LD I%,C","LD I%,D","LD I%,E","LD I%,I%","LD I%,I%l","LD H,(I%+^)","LD I%,A",
  "LD I%l,B","LD I%l,C","LD I%l,D","LD I%l,E","LD I%l,I%","LD I%l,I%l","LD L,(I%+^)","LD I%l,A",
  "LD (I%+^),B","LD (I%+^),C","LD (I%+^),D","LD (I%+^),E","LD (I%+^),H","LD (I%+^),L","HALT","LD (I%+^),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,I%","LD A,I%l","LD A,(I%+^)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD I%","ADD I%l","ADD (I%+^)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC I%","ADC I%l","ADC (I%+^)","ADC,A",
  "SUB B","SUB C","SUB D","SUB E","SUB I%","SUB I%l","SUB (I%+^)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC I%","SBC I%l","SBC (I%+^)","SBC A",
  "AND B","AND C","AND D","AND E","AND I%","AND I%l","AND (I%+^)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR I%","XOR I%l","XOR (I%+^)","XOR A",
  "OR B","OR C","OR D","OR E","OR I%","OR I%l","OR (I%+^)","OR A",
  "CP B","CP C","CP D","CP E","CP I%","CP I%l","CP (I%+^)","CP A",
  "RET NZ","POP BC","JP NZ,#","JP #","CALL NZ,#","PUSH BC","ADD *","RST 0x00",
  "RET Z","RET","JP Z,#","PFX_CB","CALL Z,#","CALL #","ADC *","RST 0x08",
  "RET NC","POP DE","JP NC,#","OUTA (*)","CALL NC,#","PUSH DE","SUB *","RST 0x10",
  "RET C","EXX","JP C,#","INA (*)","CALL C,#","PFX_DD","SBC *","RST 0x18",
  "RET PO","POP I%","JP PO,#","EX I%,(SP)","CALL PO,#","PUSH I%","AND *","RST 0x20",
  "RET PE","LD PC,I%","JP PE,#","EX DE,I%","CALL PE,#","PFX_ED","XOR *","RST 0x28",
  "RET P","POP AF","JP P,#","DI","CALL P,#","PUSH AF","OR *","RST 0x30",
  "RET M","LD SP,I%","JP M,#","EI","CALL M,#","PFX_FD","CP *","RST 0x38"
};

const char *MnemonicsXCB[256] =
{
  "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC (I%@)","RLC A",
  "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC (I%@)","RRC A",
  "RL B","RL C","RL D","RL E","RL H","RL L","RL (I%@)","RL A",
  "RR B","RR C","RR D","RR E","RR H","RR L","RR (I%@)","RR A",
  "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA (I%@)","SLA A",
  "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA (I%@)","SRA A",
  "SLL B","SLL C","SLL D","SLL E","SLL H","SLL L","SLL (I%@)","SLL A",
  "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL (I%@)","SRL A",
  "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(I%@)","BIT 0,A",
  "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(I%@)","BIT 1,A",
  "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(I%@)","BIT 2,A",
  "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(I%@)","BIT 3,A",
  "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(I%@)","BIT 4,A",
  "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(I%@)","BIT 5,A",
  "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(I%@)","BIT 6,A",
  "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(I%@)","BIT 7,A",
  "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(I%@)","RES 0,A",
  "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(I%@)","RES 1,A",
  "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(I%@)","RES 2,A",
  "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(I%@)","RES 3,A",
  "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(I%@)","RES 4,A",
  "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(I%@)","RES 5,A",
  "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(I%@)","RES 6,A",
  "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(I%@)","RES 7,A",
  "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(I%@)","SET 0,A",
  "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(I%@)","SET 1,A",
  "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(I%@)","SET 2,A",
  "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(I%@)","SET 3,A",
  "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(I%@)","SET 4,A",
  "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(I%@)","SET 5,A",
  "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(I%@)","SET 6,A",
  "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(I%@)","SET 7,A"
};



/** DAsm() ***************************************************/
/** DAsm() will disassemble the code at adress A and put    **/
/** the output text into S. It will return the number of    **/
/** bytes disassembled.                                     **/
/*************************************************************/
int DAsm(char *S,word A)
{
  char R[128],H[10],C,*P;
  const char *T;
  byte J,Offset;
  word B;

  Offset=0;
  B=A;
  C='\0';
  J=0;

  switch(RdZ80(B))
  {
    case 0xCB: B++;T=MnemonicsCB[RdZ80(B++)];break;
    case 0xED: B++;T=MnemonicsED[RdZ80(B++)];break;
    case 0xDD: B++;C='X';
               if(RdZ80(B)!=0xCB) T=MnemonicsXX[RdZ80(B++)];
               else
               { B++;Offset=RdZ80(B++);J=1;T=MnemonicsXCB[RdZ80(B++)]; }
               break;
    case 0xFD: B++;C='Y';
               if(RdZ80(B)!=0xCB) T=MnemonicsXX[RdZ80(B++)];
               else
               { B++;Offset=RdZ80(B++);J=1;T=MnemonicsXCB[RdZ80(B++)]; }
               break;
    default:   T=Mnemonics[RdZ80(B++)];
  }

  if(P=strchr(T,'^'))
  {
    strncpy(R,T,P-T);R[P-T]='\0';
    sprintf(H,"0x%02X",RdZ80(B++));
    strcat(R,H);strcat(R,P+1);
  }
  else strcpy(R,T);
  if(P=strchr(R,'%')) *P=C;

  if(P=strchr(R,'*'))
  {
    strncpy(S,R,P-R);S[P-R]='\0';
    sprintf(H,"0x%02X",RdZ80(B++));
    strcat(S,H);strcat(S,P+1);
  }
  else
    if(P=strchr(R,'@'))
    {
      strncpy(S,R,P-R);S[P-R]='\0';
      if(!J) Offset=RdZ80(B++);
      strcat(S,Offset&0x80? "-":"+");
      J=Offset&0x80? 256-Offset:Offset;
      sprintf(H,"0x%02X",J);
      strcat(S,H);strcat(S,P+1);
    }
    else
      if(P=strchr(R,'#'))
      {
        strncpy(S,R,P-R);S[P-R]='\0';
        sprintf(H,"0x%04X",RdZ80(B)+256*RdZ80(B+1));
        strcat(S,H);strcat(S,P+1);
        B+=2;
      }
      else strcpy(S,R);

  return(B-A);
}

/** DebugZ80() ***********************************************/
/** This function should exist if DEBUG is #defined. When   **/
/** Trace!=0, it is called after each command executed by   **/
/** the CPU, and given the Z80 registers.                   **/
/*************************************************************/
byte DebugZ80(Z80 *R)
{
  static const char Flags[9] = "SZ.H.PNC";
  char S[128],T[10];
  byte J,I;

  DAsm(S,R->PC.W);
  for(J=0,I=R->AF.B.l;J<8;J++,I<<=1) T[J]=I&0x80? Flags[J]:'.';
  T[8]='\0';

  printf
  (
    "AF:%04X HL:%04X DE:%04X BC:%04X PC:%04X SP:%04X IX:%04X IY:%04X I:%02X\n",
    R->AF.W,R->HL.W,R->DE.W,R->BC.W,R->PC.W,R->SP.W,R->IX.W,R->IY.W,R->I
  ); 
  printf
  ( 
    "AT PC: [%02X - %s]   AT SP: [%04X]   FLAGS: [%s]   %s: %s\n\n",
    RdZ80(R->PC.W),S,RdZ80(R->SP.W)+RdZ80(R->SP.W+1)*256,T,
    R->IFF&0x04? "IM2":R->IFF&0x02? "IM1":"IM0",
    R->IFF&0x01? "EI":"DI"
  );

  while(1)
  {
    printf("\n[Command,'?']-> ");
    fflush(stdout);fflush(stdin);

    fgets(S,50,stdin);
    for(J=0;S[J]>=' ';J++)
      S[J]=toupper(S[J]);
    S[J]='\0';

    switch(S[0])
    {
      case 'H':
      case '?':
        puts("\n***** Built-in Z80 Debugger Commands *****");
        puts("<CR>       : Break at next instruction");
        puts("= <addr>   : Break at addr");
        puts("+ <offset> : Break at PC + offset");
        puts("c          : Continue without break");
        puts("j <addr>   : Continue from addr");
        puts("m <addr>   : Memory dump at addr");
        puts("d <addr>   : Disassembly at addr");
        puts("?,h        : Show this help text");
        puts("q          : Exit Z80 emulation");
        break;

      case '\0': return(1);
      case '=':  if(strlen(S)>=2)
                 { sscanf(S+1,"%hX",&(R->Trap));R->Trace=0;return(1); }
                 break;
      case '+':  if(strlen(S)>=2)
                 {
                   sscanf(S+1,"%hX",&(R->Trap));
                   R->Trap+=R->PC.W;R->Trace=0;
                   return(1);
                 }
                 break;
      case 'J':  if(strlen(S)>=2)
                 { sscanf(S+1,"%hX",&(R->PC.W));R->Trace=0;return(1); }
                 break;
      case 'C':  R->Trap=0xFFFF;R->Trace=0;return(1); 
      case 'Q':  return(0);

      case 'M':
        {
          word Addr;

          if(strlen(S)>1) sscanf(S+1,"%hX",&Addr); else Addr=R->PC.W;
          puts("");
          for(J=0;J<16;J++)
          {
            printf("%04X: ",Addr);
            for(I=0;I<16;I++,Addr++)
              printf("%02X ",RdZ80(Addr));
            printf(" | ");Addr-=16;
            for(I=0;I<16;I++,Addr++)
              putchar(isprint(RdZ80(Addr))? RdZ80(Addr):'.');
            puts("");
          }
        }
        break;

      case 'D':
        {
          word Addr;

          if(strlen(S)>1) sscanf(S+1,"%hX",&Addr); else Addr=R->PC.W;
          puts("");
          for(J=0;J<16;J++)
          {
            printf("%04X: ",Addr);
            Addr+=DAsm(S,Addr);
            puts(S);
          }
        }
        break;

#ifdef FMSX
      case 'S':
        for(J=0;J<AY8910_CHANNELS;J++)
        {
          printf("Channel %d: Volume %d, Frequency %dHz",J,PSG.Volume[J],PSG.Freq[J]);
          if(!(PSG.R[8+(J%3)]&0x10)) printf("\n");
          else printf(", Envelope %d\n",PSG.R[8+(J%3)]&0x0F);
        }
        printf("Envelope period %dms\n",PSG.EPeriod);
        break;
#endif /* FMSX */
    }
  }

  /* Continue emulation */
  return(1);
}

#endif /* DEBUG */
