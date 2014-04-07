#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>


#include <sys/stat.h>

#include "fifo.h"

int read_buf_fifo(char_fifo_t *in, char *buf, int len) {
 int i;
 if((in->head == 0) && (in->tail == 0)) return 0;
 for(i=0;i<len;i++) {
  buf[i] = in->buf[in->head++];
  if(in->head >= in->tail) {
   in->head = 0;
   in->tail = 0;
   break;
  }
 }
 return i;
}

void write_buf_fifo(char_fifo_t *in, char *buf, int len) {
 grow_char_fifo(in, len);
 memcpy((void *)&in->buf[in->tail-len], (void *)buf, len);
}

int read_char_fifo(char_fifo_t *in) {
 int ret;
 
 if(in->head == in->tail) return -1;

 ret = in->buf[in->head++];
 if(in->head == in->tail) {
  in->head = in->tail = 0;
 }
 
 return ret;
}

void grow_char_fifo(char_fifo_t *in, int len) {
 int q;
 if(in->buf == (char *)0)
  in->buf = (char *)malloc(FIFO_CHUNK);
 else {
  q = (in->tail + len) / FIFO_CHUNK;
  in->buf = (char *)realloc(in->buf, (q+2) * FIFO_CHUNK); 
 }
 in->tail += len;
}

void shrink_char_fifo(char_fifo_t *in, int len) {
 in->head+= len;
 if(in->head >= in->tail) {
  in->head = 0;
  in->tail = 0;
 }
}

void write_char_fifo(char_fifo_t *in, char c) {
 if(in->buf == (char *)0)
  in->buf = (char *)malloc(FIFO_CHUNK);
 else {
  if(((in->tail +1 )%FIFO_CHUNK)==0) {
   in->buf = (char *)realloc(in->buf, 
     ((in->tail/FIFO_CHUNK)+2)*FIFO_CHUNK);
  }
 }
 in->buf[in->tail++] = c;
}

void free_fifo(char_fifo_t *in) {
 if(in->buf != (char *)0) {
  free(in->buf);
  in->buf = (char *)0;
 }
 in->head = in->tail = 0;
}
