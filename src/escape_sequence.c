#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include "hollow_list.h"

enum parse_state{
	NONE,
	ESCAPE,
	CSI,
	ARG,
	SEMICOLON,
};

static enum parse_state current_parse_state = NONE;

int global_foreground_color;
int global_background_color;
int color_pairs_start;
int color_pairs_red;
int color_pairs_yellow;
int color_pairs_green;
static int args[256];
static int current_arg = 0;
extern int red_background;
extern int yellow_background;
extern int green_background;

hollow_list *pairs_table = NULL;

int get_global_color(){
	return COLOR_PAIR(((global_foreground_color&7) | (global_background_color<<3)) + color_pairs_start);
}

void create_color_pairs(int pairs_start){
	int i;
	int c;

	pairs_table = create_hollow_list(sizeof(unsigned int)*8, -1);

	global_foreground_color = COLOR_WHITE;
	global_background_color = COLOR_BLACK;
	color_pairs_start = pairs_start;
	for(i = color_pairs_start, c = 0; i < color_pairs_start + 64; i++, c++){
		init_pair(i, c&7, c>>3);
		write_hollow_list(pairs_table, COLOR_PAIR(i), i, -1);
	}
	color_pairs_red = i;
	for(i = color_pairs_red, c = 0; i < color_pairs_red + 64; i++, c++){
		init_pair(i, c&7, red_background);
		write_hollow_list(pairs_table, COLOR_PAIR(i), i, -1);
	}
	color_pairs_yellow = i;
	for(i = color_pairs_yellow, c = 0; i < color_pairs_yellow + 64; i++, c++){
		init_pair(i, c&7, yellow_background);
		write_hollow_list(pairs_table, COLOR_PAIR(i), i, -1);
	}
	color_pairs_green = i;
	for(i = color_pairs_green, c = 0; i < color_pairs_green + 64; i++, c++){
		init_pair(i, c&7, green_background);
		write_hollow_list(pairs_table, COLOR_PAIR(i), i, -1);
	}
	attron(get_global_color());
}

void sgr_nothing(void){
	//do nothing :)
	//Currently listening to a song called "Hold On"
}

void sgr_reset(void){
	attrset(A_NORMAL);
	global_foreground_color = COLOR_WHITE;
	global_background_color = COLOR_BLACK;
	attron(get_global_color());
}

void sgr_bold(void){
	attron(A_BOLD);
}

void sgr_faint(void){
	attron(A_DIM);
}

void sgr_italic(void){
	attron(A_ITALIC);
}

void sgr_underline(void){
	attron(A_UNDERLINE);
}

void sgr_blink(void){
	attron(A_BLINK);
}

void sgr_reverse(void){
	attron(A_REVERSE);
}

void sgr_normal_intensity(void){
	attroff(A_DIM | A_BOLD);
}

void sgr_no_italic(void){
	attroff(A_ITALIC);
}

void sgr_no_underline(void){
	attroff(A_UNDERLINE);
}

void sgr_no_blink(void){
	attroff(A_BLINK);
}

void sgr_no_reverse(void){
	attroff(A_REVERSE);
}

void sgr_foreground_black(void){
	global_foreground_color = COLOR_BLACK;
	attron(get_global_color());
}

void sgr_foreground_red(void){
	global_foreground_color = COLOR_RED;
	attron(get_global_color());
}

void sgr_foreground_green(void){
	global_foreground_color = COLOR_GREEN;
	attron(get_global_color());
}

void sgr_foreground_yellow(void){
	global_foreground_color = COLOR_YELLOW;
	attron(get_global_color());
}

void sgr_foreground_blue(void){
	global_foreground_color = COLOR_BLUE;
	attron(get_global_color());
}

void sgr_foreground_magenta(void){
	global_foreground_color = COLOR_MAGENTA;
	attron(get_global_color());
}

void sgr_foreground_cyan(void){
	global_foreground_color = COLOR_CYAN;
	attron(get_global_color());
}

void sgr_foreground_white(void){
	global_foreground_color = COLOR_WHITE;
	attron(get_global_color());
}

void sgr_background_black(void){
	global_background_color = COLOR_BLACK;
	attron(get_global_color());
}

void sgr_background_red(void){
	global_background_color = COLOR_RED;
	attron(get_global_color());
}

void sgr_background_green(void){
	global_background_color = COLOR_GREEN;
	attron(get_global_color());
}

void sgr_background_yellow(void){
	global_background_color = COLOR_YELLOW;
	attron(get_global_color());
}

void sgr_background_blue(void){
	global_background_color = COLOR_BLUE;
	attron(get_global_color());
}

void sgr_background_magenta(void){
	global_background_color = COLOR_MAGENTA;
	attron(get_global_color());
}

void sgr_background_cyan(void){
	global_background_color = COLOR_CYAN;
	attron(get_global_color());
}

void sgr_background_white(void){
	global_background_color = COLOR_WHITE;
	attron(get_global_color());
}

static void (*sgr_functions[])(void) = {
	sgr_reset,
	sgr_bold,
	sgr_faint,
	sgr_italic,
	sgr_underline,
	sgr_blink,
	sgr_nothing,//Fast blink
	sgr_reverse,
	sgr_nothing,//Conceal
	sgr_nothing,//Crossed out

	//Font codes do nothing
	sgr_nothing, sgr_nothing, sgr_nothing, sgr_nothing, sgr_nothing,
	sgr_nothing, sgr_nothing, sgr_nothing, sgr_nothing, sgr_nothing,
	//

	sgr_nothing,//Fraktur
	sgr_nothing,//Double underline or bold off... ?
	sgr_normal_intensity,
	sgr_no_italic,
	sgr_no_underline,
	sgr_no_blink,
	sgr_nothing,//Proportional spacing
	sgr_no_reverse,
	sgr_nothing,//Conceal off
	sgr_nothing,//Not crossed out

	//Set foreground color
	sgr_foreground_black,
	sgr_foreground_red,
	sgr_foreground_green,
	sgr_foreground_yellow,
	sgr_foreground_blue,
	sgr_foreground_magenta,
	sgr_foreground_cyan,
	sgr_foreground_white,
	//

	sgr_nothing,//Set 24-bit foreground color
	sgr_foreground_white,//Set default foreground color

	//Set background color
	sgr_background_black,
	sgr_background_red,
	sgr_background_green,
	sgr_background_yellow,
	sgr_background_blue,
	sgr_background_magenta,
	sgr_background_cyan,
	sgr_background_white,
	//

	sgr_nothing,//Set 24-bit background color
	sgr_background_black,//Set default background color
	//tbh Idk what the fuck the reset of these do, so I left them blank
};

void bound_cursor_position(int *y, int *x){
	if(*y < 0)
		*y = 0;
	if(*x < 0)
		*x = 0;
	if(*x >= COLS){
		*x = COLS - 1;
	}
	if(*y >= LINES)
		*y = LINES - 1;
}

int parse_escape_char(char c, FILE *debug_file){
	int x;
	int y;
	int i;

	switch(current_parse_state){
		case NONE:
			if(c == 0x1B){
				current_parse_state = ESCAPE;
			} else {
				current_parse_state = NONE;
				current_arg = 0;
			}
			break;
		case ESCAPE:
			if(c == 'c'){
				erase();
				if(debug_file)
					fprintf(debug_file, "ESCAPE: erase\n");
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == '['){
				current_parse_state = CSI;
				args[0] = -1;
				args[1] = -1;
			} else {
				if(debug_file)
					fprintf(debug_file, "UNKNOWN ESCAPE SEQUENCE %c\n", c);
				current_parse_state = NONE;
				current_arg = 0;
			}
			break;
		case ARG:
			if(c >= '0' && c <= '9'){
				args[current_arg] = args[current_arg]*10 + c - '0';
				break;
			}
			//No break here, we continue into the CSI and SEMICOLON cases
		case CSI:
		case SEMICOLON:
			if(c == ';'){
				current_parse_state = SEMICOLON;
				if(current_arg < 255)
					current_arg++;
				args[current_arg] = -1;
			} else if(current_parse_state == CSI && c >= '0' && c <= '9'){
				args[current_arg] = c - '0';
				current_parse_state = ARG;
			} else if(current_parse_state == SEMICOLON && c >= '0' && c <= '9'){
				args[current_arg] = c - '0';
				current_parse_state = ARG;
			} else if(c == 'A'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				y -= args[0];
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move up %d\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'B'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				y += args[0];
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move down %d\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'C'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				x += args[0];
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move right %d\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'D'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				x -= args[0];
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move left %d\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'E'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				y += args[0];
				bound_cursor_position(&y, &x);
				move(y, 1);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to beginning of line %d rows down\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'F'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				y -= args[0];
				bound_cursor_position(&y, &x);
				move(y, 1);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to beginning of line %d rows up\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'G'){
				if(args[0] < 0)
					args[0] = 1;
				getyx(stdscr, y, x);
				x = args[0] - 1;
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to column %d\n", args[0]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'H'){
				if(args[0] < 0)
					args[0] = 1;
				if(args[1] < 0)
					args[1] = 1;
				args[0]--;
				args[1]--;
				bound_cursor_position(args, args + 1);
				move(args[0], args[1]);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to %d, %d\n", args[0], args[1]);
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'm'){
				if(debug_file)
					fprintf(debug_file, "ESCAPE: srg ");
				for(i = 0; i <= current_arg; i++){
					if(args[i] < 0)
						args[i] = 0;
					if(args[i] < sizeof(sgr_functions)/sizeof(sgr_functions[0])){
						if(debug_file)
							fprintf(debug_file, "%d ", args[i]);
						sgr_functions[args[i]]();
					}
				}
				if(debug_file)
					fprintf(debug_file, "\n");
				current_parse_state = NONE;
				current_arg = 0;
			} else if(c == 'K'){
				curs_set(0);
				if(args[0] < 0)
					args[0] = 0;
				switch(args[0]){
					case 0:
						clrtoeol();
						break;
					case 1:
						getyx(stdscr, y, x);
						move(y, 0);
						for(i = 0; i < x; i++){
							printw(" ");
						}
						move(y, x);
						break;
					case 2:
					case 3:
						erase();
						break;
				}
				curs_set(1);
				current_parse_state = NONE;
				current_arg = 0;
			} else {
				if(debug_file)
					fprintf(debug_file, "UNKNOWN ESCAPE CSI %c\n", c);
				current_parse_state = NONE;
				current_arg = 0;
			}
			break;
		default:
			current_parse_state = NONE;
			current_arg = 0;
	}

	if(current_parse_state == NONE)
		return 0;
	else
		return 1;
}
