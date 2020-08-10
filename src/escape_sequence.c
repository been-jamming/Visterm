#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>

enum parse_state{
	NONE,
	ESCAPE,
	CSI,
	ARG1,
	SEMICOLON,
	ARG2,
};

static int arg1;
static int arg2;
static enum parse_state current_parse_state = NONE;

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

	switch(current_parse_state){
		case NONE:
			if(c == 0x1B)
				current_parse_state = ESCAPE;
			else
				current_parse_state = NONE;
			break;
		case ESCAPE:
			if(c == 'c'){
				erase();
				if(debug_file)
					fprintf(debug_file, "ESCAPE: erase\n");
				current_parse_state = NONE;
			} else if(c == '['){
				current_parse_state = CSI;
				arg1 = 1;
				arg2 = 1;
			} else {
				if(debug_file)
					fprintf(debug_file, "UNKNOWN ESCAPE SEQUENCE %c\n", c);
				current_parse_state = NONE;
			}
			break;
		case ARG1:
		case ARG2:
			if(current_parse_state == ARG1 && c >= '0' && c <= '9'){
				arg1 = arg1*10 + c - '0';
				break;
			} else if(current_parse_state == ARG2 && c >= '0' && c <= '9'){
				arg2 = arg2*10 + c - '0';
				break;
			}
			//No break here, we continue into the CSI and SEMICOLON cases
		case CSI:
		case SEMICOLON:
			if(c == ';'){
				current_parse_state = SEMICOLON;
			} else if(current_parse_state == CSI && c >= '0' && c <= '9'){
				arg1 = c - '0';
				current_parse_state = ARG1;
			} else if(current_parse_state == SEMICOLON && c >= '0' && c <= '9'){
				arg2 = c - '0';
				current_parse_state = ARG2;
			} else if(c == 'A'){
				getyx(stdscr, y, x);
				y -= arg1;
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move up %d\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'B'){
				getyx(stdscr, y, x);
				y += arg1;
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move down %d\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'C'){
				getyx(stdscr, y, x);
				x += arg1;
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move right %d\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'D'){
				getyx(stdscr, y, x);
				x -= arg1;
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move left %d\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'E'){
				getyx(stdscr, y, x);
				y += arg1;
				bound_cursor_position(&y, &x);
				move(y, 1);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to beginning of line %d rows down\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'F'){
				getyx(stdscr, y, x);
				y -= arg1;
				bound_cursor_position(&y, &x);
				move(y, 1);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to beginning of line %d rows up\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'G'){
				getyx(stdscr, y, x);
				x = arg1 - 1;
				bound_cursor_position(&y, &x);
				move(y, x);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to column %d\n", arg1);
				current_parse_state = NONE;
			} else if(c == 'H'){
				arg1--;
				arg2--;
				bound_cursor_position(&arg1, &arg2);
				move(arg1, arg2);
				if(debug_file)
					fprintf(debug_file, "ESCAPE: move to %d, %d\n", arg1, arg2);
				current_parse_state = NONE;
			} else {
				if(debug_file)
					fprintf(debug_file, "UNKNOWN ESCAPE CSI %c\n", c);
				current_parse_state = NONE;
			}
			break;
		default:
			current_parse_state = NONE;
	}

	if(current_parse_state == NONE)
		return 0;
	else
		return 1;
}
