%token T_68K T_FM T_PSG T_Z80 T_BREAK T_STOP T_CONT T_RESET T_INT T_JUMP
%token T_DUMP T_DIS T_LOAD T_POKE T_CLEAR T_HELP T_EXIT T_REGS T_PAL T_NTSC
%token T_SCRIPT T_BANK T_SYSTYPE CONSTANT STRING_LITERAL T_VERSION T_HARD
%token T_S T_ERROR T_8BIT T_COMMENT T_WAIT T_WATCH 

%start command

%{
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <SDL.h> 
 #include <errno.h>
 #include <sys/stat.h>
 #include "link.h"
 #include "version.h"
 #include "fifo.h"

 #include "vt100.h"
 #include "async.h"
 #include "main.h"
 #include "app.h"
 #include "Z80.h"
 #include "emu.h"

 #include "lang.h"
 #include "Debug.h"

 #include "snd.h"

 #include "sndp.h"

#define WAIT_J_LINE \
  for(;;) \
   if((display_line = get_j_line())!=(char *)0) { \
    d_put(display_line); \
    break; \
   }

int ntsc_pal = NTSC;
value_t *value, *value2, *value3;
int p_done;
char disassembly[WIDTH];
int i, j;
void *p;
node_t *store_node;

node_list_t *node_list = (node_list_t *)0;
node_t *poke_list;

struct stat qstat;
unsigned int address_counter=0;
unsigned int dump_address_counter=0;
unsigned int dump_68k_address_counter=0;

FILE *fp;

const char help_z80[] = {
 "\33[0mscript \"<file>\"                    run commands from a file\n"
 "\33[42m68k                                switch to 68k mode\n"
 "\33[0mbreak <location/clear>             set breakpoint\n"
 "\33[42mcont                               continue execution\n"
 "\33[0ms                                  step execution\n"
 "\33[42mstop                               stop execution\n"
 "\33[0mreset                              reset execution\n"
 "\33[42mreset hard                         hard reset\n"
 "\33[0msystype                            display system type\n"
 "\33[42msystype <pal/ntsc>                 interrupt execution\n"
 "\33[0mjump <address>                     jump to address\n"
 "\33[42mdump <address>                     dump memory\n"
 "\33[0mdump                               dump memory\n"
 "\33[42mwait                               wait one frame\n"
 "\33[0mregs                               display registers\n"
 "\33[42mdis <address>                      disassemble at address\n"
 "\33[0mdis                                disassemble\n"
 "\33[42mload \"<file>\" <location>           load file into memory\n"
 "\33[0mpoke <address>:<value>,...         put values into memory\n"
 "\33[42mpoke xb <address>:<value>,...      put 8bit values into 68k memory\n"
 "\33[0mbank <address>                     change bank\n"
 "\33[42mbank                               show bank\n"
 "\33[0mclear                              clear memory\n"
 "\33[42mwatch <address>:<value>,[timeout]  watch address for change to value\n"
 "\33[0mwatch                              watch nothing\n"
 "\33[42mhelp                               show help\n"
 "\33[0mversion                            version info\n"
 "\33[42mexit                               quit debugger\n"
};

const char help_68k[] = {
 "\33[0mscript \"<file>\"                    run commands from a file\n"
 "\33[42mz80                                switch to z80 mode\n"
 "\33[0mdump <address>                     dump range of memory\n"
 "\33[42mload \"<file>\" <location>           load file into memory\n"
 "\33[0mpoke <address>:<value>,...         put values into memory\n"
 "\33[42mclear                              clear memory\n"
 "\33[0mhelp                               show help\n"
 "\33[42mversion                            version info\n"
 "\33[0mexit                               quit debugger\33[0m\n"
};

const char help_fm[] = {
 "\33[0mscript \"<file>\"                    run commands from a file\n"
 "\33[42mz80                                switch to z80 mode\n"
 "\33[0m68k                                switch to 68k mode\n"
 "\33[42mpsg                                switch to psg mode\n"
 "\33[0mdump <chip>:<address>              dump fm registers\n"
 "\33[42mpoke <chip>:<address>:<value>,...  put values into fm registers\n"
 "\33[0mclear                              clear registers\n"
 "\33[42mhelp                               show help\n"
 "\33[0mversion                            version info\n"
 "\33[42mexit                               quit debugger\n"
};

const char help_psg[] = {
 "\33[0mscript \"<file>\"                    run commands from a file\n"
 "\33[42mz80                                switch to z80 mode\n"
 "\33[0m68k                                switch to 68k mode\n"
 "\33[42mfm                                 switch to fm mode\n"
 "\33[0mpoke <value>                       put value into psg register\n"
 "\33[42mhelp                               show help\n"
 "\33[0mversion                            version info\n"
 "\33[42mexit                               quit debugger\n"
};

int cluster = 0;
const char line_colors[6][30] = {
 "red",
 "green",
 "blue",
 "yellow",
 "cyan",
 "black"
};

int yylex(void);

int input_error = 0;


void dump(void) {
 i= 0;
 if(mode == MZ80){
  if(dump_address_counter > 0xffff) {
   d_puts("address out of range");
   i = 80;
  }
 } else {
  if(dump_68k_address_counter>0xffffff) {
   d_puts("address out of range");
   i = 80;
  }
 }

 p_done = 0;
 for(;i<40;i++) {
  if((i%2)==0) 
   d_put("\33[0m");
  else
   d_put("\33[42m");
  
  if(mode == MZ80)
   d_printf("0x%04x: ", dump_address_counter);
  else
   d_printf("0x%06x: ", dump_68k_address_counter);
  for(j=0;j<(mode == MZ80 ? 16 : 8);j++) {
   if(mode == MZ80) {
    d_printf("%02x ", RdZ80(dump_address_counter));
    dump_address_counter++;
   } else { 
    d_printf("%02x%02x ", m68k_ram[dump_68k_address_counter],
                          m68k_ram[dump_68k_address_counter+1]);
    dump_68k_address_counter+=2;
   }
  }
  if(mode == MZ80) {
   if(dump_address_counter > 0xffff - 0x10)
    p_done = 1;
  } else {
   if(dump_68k_address_counter > 0xffffff -0x10)
    p_done = 1;
  }
  d_putc('\n');  
  if(p_done == 1) break;
 }
}


void disassemble(void) {
 int i, j, k;
 for(i=0;i<40;i++) {
  if(address_counter <= 0xffff) {
   if((z80.Trap == address_counter) && (z80.Trap != -1))
    d_printf("\33[33m0x%04x\33[0m: ", address_counter);
   else
    d_printf("0x%04x: ", address_counter);
   j=DAsm(disassembly, address_counter);
   for(k=0;k<4;k++) {
    if(k<=j)
     d_printf("%02x ", RdZ80(address_counter+k)); 
    else
     d_printf("   ");
   }
   if(j+address_counter <=0xffff)
    d_printf("\33[36m%s\33[0m\n", disassembly);
   address_counter+=j;
  }
 }

}


node_t *add_node(int top, char *oper, value_t *value, int nops, ...) {
 va_list ap; 
 node_t *new, *child;
 int i;

 if((new = (node_t *)malloc(sizeof(node_t))) == NULL){
  fprintf(stderr, "out of memory 1");
  exit(0);
 }

 new->op = oper;
 new->value = value; 
 new->child_count = nops;

 if((new->children = (void **)malloc(nops * sizeof(void *))) == NULL) {
  fprintf(stderr, "out of memory 2");
  exit(0);
 } 

 new->children = (void **)0;

 if(nops!=0) 
  if((new->children = (void **)malloc(sizeof(void *)*nops))==NULL) {
   fprintf(stderr, "out of memory 3");
   exit(0);
  }


 va_start(ap, nops);
 for(i=0;i<nops;i+=1) {
  new->children[i] = (void *)va_arg(ap, void *);
 }

 if(top != T) {
  for(i=0;i<nops;i+=1) {
   child = (node_t *)new->children[i];
   child->parent = (void *)new;
  }
 } 
 add_node_to_list( new);

 return new;
}

void add_node_to_list(node_t *in) {
 node_list_t *new;

 if((new = (node_list_t *)malloc(sizeof(node_list_t)))==NULL) {
  fprintf(stderr, "out of memory 4");
  exit(0);
 }

 if(node_list == (node_list_t *)0) {
  node_list = new;
  INIT_LIST_HEAD(&node_list->node);
 } else 
  list_add(&new->node,&node_list->node);

 new->store = in;
}


%}
%union {
 int num;
 void *p;
}

%%

command
	: T_EXIT { prog_done = 1; }
	| T_VERSION { d_putc('\n'); d_put(VERSION_ANS); }
	| T_ERROR {
	 input_error = 1;
	}
	| T_COMMENT
	| T_WATCH {
	 watcher = -1;
	}
	| T_WATCH CONSTANT ':' CONSTANT {
         value = (value_t *)$<p>2;
	 value2= (value_t *)$<p>4;

	 watcher = value->datum.ivalue;
	 watch_for=value2->datum.ivalue;
	 watch_timeout = -1;

	 free(value);
	 free(value2);
	}
	| T_WATCH CONSTANT ':' CONSTANT ',' CONSTANT {
         value = (value_t *)$<p>2;
	 value2= (value_t *)$<p>4;
         value3= (value_t *)$<p>6;

	 watcher = value->datum.ivalue;
	 watch_for=value2->datum.ivalue;
	 watch_timeout = value3->datum.ivalue;
         watch_start = SDL_GetTicks();

	 free(value);
	 free(value2);
	 free(value3);

	}
	| T_WAIT {
	 if(z80.Trace == 0) {
	  wait_state = 1;
	  while(wait_state!=0) SDL_Delay(10);
	 } else
	  d_puts("cannot wait while stopped");
	  
	}
	| T_CLEAR {
         switch(mode) {
          case MZ80: 
	   memset(z80_ram, 0, 0x2000);
           d_puts("memory cleared\n");
	   break;
	  case M68k:
	   memset(m68k_ram, 0, 0xffffff);
           d_puts("memory cleared\n");
	   break;
	  case MFM:
	   for(i=0;i<0xff;i++) {
	    YM2612Write(0, 0, i);
	    YM2612Write(0, 1, 0x00);
	    YM2612Write(0, 2, i);
	    YM2612Write(0, 3, 0x00);
	   }
	   d_puts("memory cleared\n");
	   break;
	  case MPSG:
	   d_puts("can't do this here");
	   break;
	 }
	}
	| T_SCRIPT STRING_LITERAL {
	 value= (value_t *)$<p>2;	 
	 if((fp = fopen(value->datum.string, "rb"))<=0) {
          d_printf("%s: %s\n", value->datum.string, strerror(errno));
	 } else 
          script_fp = fp; 

	 free(value->datum.string);
	 free(value);
       	}
	| T_BREAK T_CLEAR {
	 if(mode != MZ80) 
	  d_puts("can't do this here");
	 else {
	  z80.Trap = -1;
	  d_puts("breakpoint cleared");
	 }
	}
	| T_BREAK CONSTANT {
	 value = (value_t *)$<p>2;
	 if(mode != MZ80) 
	  d_puts("can't do this here"); 
	 else {
	  if(value->datum.ivalue > 0xffff)
	   d_puts("address out of range");
	  else {
	   z80.Trap = value->datum.ivalue;
	   d_printf("breakpoint set to 0x%04x\n", value->datum.ivalue);
	  }
	 }
	 free(value);
       	}
	| T_CONT {
	 if(mode != MZ80)
	  d_puts("can't do this here");
	 else {
	  if(z80.Trace == 1) {
	   z80.Trace = 0;
	   WAIT_J_LINE;
	  }
	 } 
       	}
	| T_S  {
	 if(mode != MZ80)
	  d_puts("can't do this here");
	 else {
	  if(z80.Trace == 1) {
	   z80.step = GO;
	   WAIT_J_LINE;
	   WAIT_J_LINE; 
	  }
	 }
	}
	| T_STOP { 
	 if(mode != MZ80)
	  d_puts("can't do this here");
	 else {
	  if(z80.Trace == 0) {
	   z80.Trace = 1;
	   WAIT_J_LINE;
	  }
	 }

	}
	| T_RESET {
	 do_reset = GO;
	 d_puts("z80 reset (nmi)");
       	}
	| T_RESET T_HARD {
         do_hard_reset = GO;
	 d_puts("z80 hard reset");
	}	 
	| T_INT { 
	 d_putc('\n');
	}
	| T_SYSTYPE {
	 d_printf("system is %s\n", (ntsc_pal == NTSC ? "NTSC" : "PAL" ));
	}
	| T_SYSTYPE pal_ntsc {
	 if(ntsc_pal == NTSC) {
	  YM2612Init(1, NTSC_CLOCK / 7, SOUND_SAMPLERATE, NULL, NULL);
	  SN76496Init(0,NTSC_CLOCK/15, 0, SOUND_SAMPLERATE);
	  d_puts("system is NTSC");
	 } else {
	  YM2612Init(1, PAL_CLOCK / 7, SOUND_SAMPLERATE, NULL, NULL);
	  SN76496Init(0,PAL_CLOCK/15, 0, SOUND_SAMPLERATE);
	  d_puts("system is PAL"); 
	 }
	 status_dirty = 1;
       	}
	| T_JUMP CONSTANT { 
	 value = (value_t *)$<p>2;

         z80.PC.W = value->datum.ivalue;
	 d_printf("PC set to 0x%04x\n", z80.PC.W);

	 free(value);
	}
	| T_DUMP {
	 dump();
	}
	| T_DUMP CONSTANT { 
	 value = (value_t *)$<p>2;

	 p_done = 0;
	 if(mode == MZ80) {
	  if(value->datum.ivalue > 0xffff) {
	   d_puts("address out of range");
	   p_done = 1;
	  } else {
	   dump_address_counter = value->datum.ivalue/0x10;
	   dump_address_counter*=0x10;
	  }
	 } else {
	  if(value->datum.ivalue > 0xffffff) {
	   d_puts("address out of range");
	   p_done = 1;
	  } else {
	   dump_68k_address_counter = value->datum.ivalue/0x10;
	   dump_68k_address_counter*=0x10;
	  }
	 }

	 if(p_done == 0)
	  dump();
	 
	 free(value);
	}
	| T_REGS {
	 if(mode != MZ80) 
	  d_puts("can't do this here");
	 else {
	  do_regs = GO; 
	  WAIT_J_LINE;
	  WAIT_J_LINE;
	 }
	}
	| T_DIS {
	 if(mode != MZ80) {
	  d_puts("can't do this here");
	 } else {
	  disassemble();
	 }	 
	}
	| T_DIS CONSTANT  { 
         value = (value_t *)$<p>2;

	 if(mode != MZ80) 
	  d_puts("can't do this here");
	 else {
	  if(value->datum.ivalue>0xffff) {
	   d_puts("address out of range");
	  } else {
	   address_counter = value->datum.ivalue;
           
	   disassemble();
	  }
	 } 


	 free(value);
	}
	| T_LOAD STRING_LITERAL CONSTANT { 
	 value= (value_t *)$<p>2;	 
	 value2=(value_t *)$<p>3;

	 if((mode != MZ80) && (mode != M68k)) {
	  d_puts("can't do this here");
	 } else {
	  if((value2->datum.ivalue<0)||(value2->datum.ivalue>
	     (mode == MZ80 ? 0x1fff : 0xffffff)))
	   d_puts("address out of range");
          else {
 	   if((fp = fopen(value->datum.string, "rb"))<=0) {
            d_printf("%s: %s\n", value->datum.string, strerror(errno));
	   } else  {
	    /* do stuff */ 
	    fstat(fileno(fp),&qstat);
            if(mode == MZ80) {
	     if((qstat.st_size-value2->datum.ivalue)>0xffff)
	      d_puts("file too large");
	     else {
              i = fread(&z80_ram[value2->datum.ivalue],1,qstat.st_size, fp);
	     }
	    } else {
	     if((qstat.st_size-value2->datum.ivalue)>0xffffff)
	      d_puts("file too large"); 
	     else {
              i = fread(&m68k_ram[value2->datum.ivalue],1,qstat.st_size,fp);
	     }
	    }  
	    if(i<=0) {
             d_printf("%s: %s\n", value->datum.string, strerror(errno)); 
	    } else
	     d_printf("%d bytes loaded into memory\n", i);
	    fclose(fp);
	   }
	  }	  
	 }
	 free(value->datum.string);
	 free(value);
	 free(value2);

	}
	| T_POKE T_8BIT CONSTANT ':' value_list {
         poke_list = add_node(S, "POKE_LIST", (void *)0, 1, $<p>5);
	 value = (value_t *)$<p>3;
	 if((mode == MFM) || (mode == MPSG))
	  d_puts("can't do this here");
	 else {
          i = value->datum.ivalue;
	  store_node = (node_t *)poke_list->children[0];
          for(;;) {
	   if(mode == MZ80) 
	    WrZ80((i++)&0xffff, store_node->value->datum.ivalue&0xff);
	   else 
            m68k_ram[(i++)&0xffffff]=(store_node->value->datum.ivalue&0xff);
	   
           if(strncmp(store_node->op, "vl:0", 4)==0) break;
	   store_node = (node_t *)store_node->children[0];
	  }

	 }

	 free(value);
	}
	| T_POKE CONSTANT ':' value_list { 
	 poke_list = add_node(S, "POKE_LIST", (void *)0, 1, $<p>4);
         value = (value_t *)$<p>2; 

	 if((mode == MFM) || (mode == MPSG))
	  d_puts("can't do this here");
	 else {
          i = value->datum.ivalue;
	  store_node = (node_t *)poke_list->children[0];
          for(;;) {
	   if(mode == MZ80) {
	    WrZ80((i++)&0xffff, store_node->value->datum.ivalue&0xff);
	   }else {
	    m68k_ram[(i++)&0xffffff]=
	     (store_node->value->datum.ivalue&0xff00)>>8;
            m68k_ram[(i++)&0xffffff]=(store_node->value->datum.ivalue&0xff);
	   }
           if(strncmp(store_node->op, "vl:0", 4)==0) break;
	   store_node = (node_t *)store_node->children[0];
	  }
         }
         free(value);
	}
	| T_BANK {
	 d_printf("Bank: 0x%06x\n", bank<<15);

	}
	| T_BANK CONSTANT {
	 value = (value_t *)$<p>2;
	 if(mode!=MZ80) 
	  d_puts("can't do this here");
	 else {  
	  bank = value->datum.ivalue>>15;
	  status_dirty = 1;
	 }
	 free(value);
	}
	| T_DUMP CONSTANT ':' CONSTANT {
       	}
	| T_68K { mode = M68k; } 
	| T_Z80 { mode = MZ80; }
	| T_HELP {
	 d_putc('\n');
	 switch(mode) {
	  case M68k:
	   d_put(help_68k);
	   break;
	  case MFM:
	   d_put(help_fm);
	   break;
	  case MPSG:
	   d_put(help_psg);
	   break;
	  case MZ80:
	   d_put(help_z80);
	   break;
	 }
	}
	| error { input_error = 1; }
	;

pal_ntsc
	: T_PAL { ntsc_pal = PAL; }
	| T_NTSC { ntsc_pal = NTSC; }
	;

value_list
	: CONSTANT {
	 $<p>$ = (void *)add_node(T, "vl:0", (void *)$<p>1, 0);
	}
	| CONSTANT ',' value_list {
         $<p>$ = (void *)add_node(T, "vl:1", (void *)$<p>1, 1, $<p>3);
	}
	;
	

%%

#include <stdio.h>

void yyerror(const char *s) {
 printf("syntax error\n");
}
