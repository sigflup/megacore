
#define FIFO_CHUNK	4096

typedef struct {
 char *buf;
 int head, tail;
} char_fifo_t;

int read_buf_fifo(char_fifo_t *in, char *buf, int len);
void write_buf_fifo(char_fifo_t *in, char *buf, int len);

int read_char_fifo(char_fifo_t *in);
void grow_char_fifo(char_fifo_t *in, int len);
void shrink_char_fifo(char_fifo_t *in, int len);
void write_char_fifo(char_fifo_t *in, char c);

void free_fifo(char_fifo_t *in);
