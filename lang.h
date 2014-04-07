#define MAX_BUF	1024

enum {
 S,
 E,
 T
};

enum {
 STRING,
 FLOAT,
 INT,
 IDENT,
 V_TRUE,
 V_FALSE,
 V_NULL
};

enum {
 NTSC,
 PAL
};

typedef struct {
 int type;
 union {
  char *string;
  char *ident;
  float fvalue;
  unsigned int ivalue; 
 } datum; 
} value_t;

typedef struct {
 void *parent; 
 char *op;
 value_t *value;
 int child_count;
 void **children;
} node_t;

typedef struct {
 struct list_head node;
 node_t *store;
} node_list_t;

extern node_list_t *node_list;

extern node_t *poke_list;

extern int ntsc_pal;
extern int line_pos;
extern int input_error;

void add_node_to_list(node_t *in); 
node_t *add_node(int top, char *oper, value_t *value, int nops, ...);

