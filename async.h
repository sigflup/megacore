extern char_fifo_t in_fifo, out_fifo, display_fifo;

void sig_child(int sig);
Uint32 sig_alarm(Uint32 interval);
void callback(char c, struct term_t *win); 
int d_getc(void);
