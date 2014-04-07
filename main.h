#define WIDTH	80
#define HEIGHT  50	

#define PRINTF_BUF_LEN	16384
#define BUFFER_LEN	1024
#define SPEED		8
#define MAX_TIMER	64	

#define BOX_W		200
#define BOX_H		100

#define BACK_SHADE	0x9f

void d_drawscreen(void);
extern int cursor;
extern term_t main_term;
extern int prog_done;

extern SDL_mutex *out_fifo_mutex;
extern SDL_mutex *in_fifo_mutex;
extern SDL_mutex *display_fifo_mutex;

int d_write(unsigned char *buf, int l);
void d_printf(char *fmt, ...);
void d_puts(char *);
void d_putc(int c);

void j_printf(char *fmt, ...);
void j_puts(char *);
void j_putc(int c);
