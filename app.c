#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include "link.h"

#include "fifo.h"

#include "vt100.h"
#include "async.h"
#include "main.h"
#include "app.h"
#include "Z80.h"

#include "lang.h"

#include "head.h"

#include "emu.h"

Uint32 last_draw_status = 0;

const char mode_text[4][4] = {
 "68k","z80","fm","psg"
};

int mode = MZ80;

char *display_line;
char line[MAX_LINE];
char *lines[MAX_LINES];
char j_line[MAX_LINE];
char j_line_i = 0;
FILE *script_fp = (FILE *)0;

int lines_end = 0;
int cur_line = 0;
int status_dirty = 1;

void yyparse(void);

char *get_j_line(void) {
 int c;
 SDL_mutexP(display_fifo_mutex);
 c = read_char_fifo(&display_fifo);
 SDL_mutexV(display_fifo_mutex);
 if(c!=-1) {
  j_line[j_line_i++] = c;
  if(c == '\n') {
   j_line[j_line_i] = 0;
   j_line_i = 0;
   return j_line;
  } 
 } else
  return (char *)0;

 return (char *)0;
}

/* rline {{{ */
void rline(char *buf, int l, char *p) {
 int i,c,d,j, c2, c3;
 int cur = 0;
 char *display_line;
 int done;
 int insert = 0;
 char *tmp_line;

 for(i=0;i<l;i++)
  buf[i] = 0;
 for(i=0;;i++) {
  if(p[i] != 0) 
   d_putc(p[i]);
  else
   break;
 }
 done = 0;
 while(done!=1) {
  for(;;) { 
   SDL_Delay(10);
   c = d_getc();
   if(c!=-1) break;
   if(status_dirty == 1) 
    draw_status();
   if((display_line = get_j_line())!=(char *)0) {
    /* display j line, line */
    d_putc('\n');
    d_put(display_line);
    d_put(p);
    d_put(line);
    if(cur != strlen(line))
     d_printf("\33[%dD", strlen(line) - cur);
   }
  }
  switch(c) {
   case '\b':
    if(cur!=0) {
     cur--;
     for(i=cur;;i++) {
      buf[i] = buf[i+1];
      if(buf[i] == 0) break;
     }
     d_putc('\b');
     for(i=cur;;i++) {
      if(buf[i] == 0) break;
      d_printf("\33[0m%c", buf[i]);
     }
     d_put("\33[K");
     for(i=cur;;i++) {
      if(buf[i] == 0) break;
      d_put("\33[D");
     }
    }
    break;
   case '\r':
    done = 1;
    break;
   case '\33':
    for(;;) {
     c2 = d_getc();
     if(c2!=-1) break;
    }
    if(c2 == '[') {
     for(;;) {
      d = d_getc();
      if(d!=-1) break;
     }
     switch(d) {
      case 'D':
       cur--;
       if(cur < 0) 
	cur = 0;
       else
        d_put("\33[D");
       break;
      case 'C':
       if(cur!=(80-strlen(p)-2)) {
        cur++;
        if(buf[cur-1] == 0) 
 	 cur--;
        else
  	 d_put("\33[C");
       }
       break;
      case 64:
       insert^=1;
       break; 
      case 'A':
       if((tmp_line = up_history())!=(char *)0) {
        d_printf("\33[%dD%s%s\33[K", WIDTH, p, tmp_line); 
	cur= strlen(tmp_line);
	snprintf(buf, l, "%s");
       }
       break;
      case 'B':
       if((tmp_line = down_history())!=(char *)0) {
        d_printf("\33[%dD%s%s\33[K", WIDTH, p, tmp_line);
	cur = strlen(tmp_line);
	snprintf(buf, l, "%s");
       }	

       break;
     }
    } 
    break;
   default:
    if(insert == 0) {
  
     if(strlen(buf)!=(80-strlen(p)-2)) {
      if(buf[cur] == 0) {
       buf[cur++] = c;
       buf[cur] = 0;
       d_printf("\33[0m%c", c);
      } else {
       for(j=cur;;j++)
        if(buf[j] == 0) break;

       for(i=j-1;i>=cur;i--)
        buf[i+1] = buf[i];

       buf[cur] = c;

       for(i=cur;;i++) {
        if(buf[i] == 0) break;
        d_putc(buf[i]);
       }
       for(i=cur+1;;i++) {
        if(buf[i] == 0) break;
        d_put("\33[D");
       }
       cur++;
      }
     }

    } else {
     /* insert code here */
    }
    break;
  }
 }
 d_putc('\n');

}
/* }}} */

void add_history(char *line) {
 lines[lines_end%MAX_LINES] = strdup(line);
 lines_end++;
 cur_line = lines_end-1;
}


char *up_history(void) {
 printf("UP: %d %d\n", cur_line, lines_end);
 char *ret;
 ret = lines[cur_line--];
 if(cur_line<0)
  cur_line++;
 return ret;
}

char *down_history(void) {
 char *ret;
 printf("DOWN: %d %d\n", cur_line, lines_end);
 if((cur_line == (lines_end-1))||
    ((cur_line==0)&&(lines_end==0)))
  return "";
 return lines[++cur_line];
}

void draw_status(void) {
 return;
 if((SDL_GetTicks() - last_draw_status)<120) return;
 last_draw_status = SDL_GetTicks();

 d_put("\33[s\33[50;0f");

 d_printf("\33[30;47m   Z80 %s |",(z80.Trace == 1?"STOPPED":"RUNNING") );
 d_printf(" %s |", (ntsc_pal == NTSC ? "NTSC" : "PAL ") );
 d_printf(" 68k BANK 0x%06x ", bank<<15);
 d_put("\33[0m\33[u");
 status_dirty = 0;
}

int app(void *a) {
 int i;
 unsigned char d;
 int script_done, cur;
 char buf[100];

 for(i=0;i<HEAD_ANS_len;i++)
  d_putc(HEAD_ANS[i]);

 init_emu_start = 1;

 for(;;) {
  if(init_emu_done == 1) 
   break;
  SDL_Delay(10);
 }

 /* wait for "z80 emulation stopped" */
 for(;;) {
  if((display_line = get_j_line())!=(char *)0) {
   d_put(display_line);
   break;
  }
 }

 d_put("\33[s\33[0;49r\33[u");

 for(;;) {
  if((display_line = get_j_line())!=(char *)0)
   d_put(display_line);
  snprintf(buf, 100, "\33[0m\33[41m%s\33[0m>", mode_text[mode]);
  rline(line, MAX_LINE, buf);
  if(line[0]!=0) {
   add_history(line);
   line_pos = 0;
   input_error = 0;
   yyparse();
   if(input_error == 1) 
    d_puts("ERROR");
  }
  if(script_fp != (FILE *)0) {
   d_puts("loaded script");
   for(script_done = 0;;) {
    cur = 0;
    for(;;) {
     if(status_dirty == 1)
      draw_status();
     if(fread(&d, 1, 1, script_fp)!=1)
      if(feof(script_fp)) {
       d = '\n';
       script_done= 1;
      }
     if(d == '\n') {
      line[cur] = 0;
      if(line[0] !=0) {
       d_printf("\33[41m%s\33[0m>", mode_text[mode]);
       d_puts(line);
       line_pos = 0;
       input_error = 0;
       yyparse();
      }
      break;
     }
     line[cur++] = d;
    } 
    if(script_done == 1) break;
   }
   fclose(script_fp);
   script_fp = (FILE *)0;
  }
 }
 return 0;
}
