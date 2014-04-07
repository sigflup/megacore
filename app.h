
#define MAX_LINE	256	
#define MAX_LINES	256

enum {
 M68k =0,
 MZ80,
 MFM,
 MPSG
};

extern FILE *script_fp;
extern const char mode_text[4][4];
extern char line[MAX_LINE];
extern int mode;
extern char *display_line;
extern int status_dirty;


void draw_status(void);
char *up_history(void);
char *down_history(void);
char *get_j_line(void);

int app(void *a);
