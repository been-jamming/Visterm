int parse_escape_char(char c, FILE *debug_file);

int get_global_color();
void bound_cursor_position(int *y, int *x);
void create_color_pairs(int pairs_start);

extern int global_foreground_color;
extern int global_background_color;
extern hollow_list *pairs_table;
extern int auto_margins;

extern int color_pairs_start;
extern int color_pairs_red;
extern int color_pairs_yellow;
extern int color_pairs_green;

extern int global_attr;
