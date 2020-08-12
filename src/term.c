#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <complex.h>
#include <SDL2/SDL.h>
#include "audio_monitor.h"
#include "fft.h"
#include "hollow_list.h"
#include "escape_sequence.h"

int open_terminal();
double *averages;
FILE *debug_file = NULL;
int do_ctrl_c = 0;
int in_escape_sequence = 0;
int red_background;
int yellow_background;
int green_background;

static int pty_fd;

void unget_str(char *str){
	char *str_start;

	str_start = str;
	str += strlen(str);
	while(str != str_start){
		str--;
		ungetch(*str);
	}
}

uint64_t get_nanoseconds(struct timespec t){
	return 1000000000ULL*t.tv_sec + t.tv_nsec;
}

void print_bash_output(char *str){
	int y;
	int x;

	if(debug_file)
		fprintf(debug_file, "PRINT: \"%s\"\n", str);
	while(*str){
		if(*str == 0x1B || in_escape_sequence){
			in_escape_sequence = parse_escape_char(*str, debug_file);
		} else if(*str == '\a')
			fputc('\a', stdout);
		else if(*str == '\b'){
			getyx(stdscr, y, x);
			x--;
			bound_cursor_position(&y, &x);
			move(y, x);
		} else if(*str == '\r'){
			getyx(stdscr, y, x);
			move(y, 0);
		} else if(*str == '\f'){
			erase();
			move(0, 0);
		} else if(*str == '\n'){
			getyx(stdscr, y, x);
			move(y, COLS - 1);
			printw("\n");
		} else
			printw("%c", (int) *str);
		str++;
	}
}

static void blank_free(int i){

}

void exit_terminal(){
	refresh();
	endwin();
	SDL_Quit();
	close(pty_fd);
	free(frequencies);
	free(samples);
	if(pairs_table)
		free_hollow_list(pairs_table, blank_free);
	if(debug_file)
		fclose(debug_file);
	printf("Terminal closed\n");

	exit(0);
}

void ctrl_c(int sig){
	do_ctrl_c = 1;
}

double average_amplitudes(int last_freq_index, int freq_index){
	double sum = 0;
	int i;

	for(i = last_freq_index + 1; i <= freq_index; i++){
		sum += cabs(frequencies[freq_index]);
	}

	return sum/(freq_index - last_freq_index);
}

void set_char_background(int y, int x, int background_color_start){
	int prev_curs_x;
	int prev_curs_y;
	chtype char_data;
	chtype color_data;
	int pair_num;
	int new_pair_num;

	getyx(stdscr, prev_curs_y, prev_curs_x);
	char_data = mvinch(y, x);
	color_data = char_data&A_COLOR;
	pair_num = read_hollow_list(pairs_table, color_data, -1);
	if(pair_num == -1)
		return;
	if(pair_num < color_pairs_red && pair_num >= color_pairs_start){
		new_pair_num = background_color_start + pair_num - color_pairs_start;
	} else if(pair_num < color_pairs_yellow){
		new_pair_num = background_color_start + pair_num - color_pairs_red;
	} else if(pair_num < color_pairs_green){
		new_pair_num = background_color_start + pair_num - color_pairs_yellow;
	} else if(pair_num > color_pairs_green){
		new_pair_num = background_color_start + pair_num - color_pairs_green;
	} else {
		return;
	}
	mvaddch(y, x, (char_data&~A_COLOR) | COLOR_PAIR(new_pair_num));
	move(prev_curs_y, prev_curs_x);
}

void draw_to_right(int y, int x, int end_x, int background_color_start){
	while(x <= end_x){
		set_char_background(y, x, background_color_start);
		x++;
	}
}

void draw_bar(int x, int y, int term_width){
	if(x > term_width - 1)
		x = term_width - 1;
	draw_to_right(y, 0, term_width - 2 - x, color_pairs_start);
	draw_to_right(y, term_width - 1 - x, term_width - 2 - x*2/3, color_pairs_red);
	draw_to_right(y, term_width - 1 - x*2/3, term_width - 2 - x/3, color_pairs_yellow);
	draw_to_right(y, term_width - 1 - x/3, term_width - 1, color_pairs_green);
}

void update_visualizer(){
	int term_height;
	int term_width;
	double freq_change;
	double freq;
	double amplitude;
	int freq_index;
	int last_freq_index = 0;
	int y;
	int x;
	int orig_y;
	int orig_x;

	scrollok(stdscr, 0);
	getyx(stdscr, orig_y, orig_x);

	SDL_LockAudioDevice(recording_device_id);
	dfft2_float(frequencies, samples, 10);
	SDL_UnlockAudioDevice(recording_device_id);

	term_height = LINES;
	term_width = COLS;
	freq_change = 940.0/term_height;
	for(y = 0; y < term_height; y++){
		freq = freq_change*y + 60;
		freq_index = (int) (freq/11025*1024);
		amplitude = average_amplitudes(last_freq_index, freq_index)*0.7 + averages[y]*0.3;
		//amplitude = cabs(frequencies[freq_index])*0.4 + averages[y]*0.6;
		averages[y] = amplitude;
		x = amplitude*term_height;
		draw_bar(x, y, term_width);
		//mvchgat(y, 0, -1, A_NORMAL, 2, NULL);
		//mvchgat(y, x, -1, A_NORMAL, 1, NULL);
	}

	move(orig_y, orig_x);
	scrollok(stdscr, 1);
}

int main(int argc, char **argv){
	char buffer[8192] = {0};
	int key_press;
	char current_char;
	int chars_read;
	struct timespec last_time;
	struct timespec current_time;
	struct timespec sleep_time;
	uint64_t last_nanoseconds;
	uint64_t current_nanoseconds;
	struct sigaction sigint_action;
	fd_set readable;
	struct timeval no_wait;
	sigset_t block_sigint;

	if(argc >= 2 && !strcmp(argv[1], "--debug")){
		printf("Opening in debug mode\n");
		debug_file = fopen("visterm_debug", "w");
	}

	if(audio_monitor_setup(44100/4, 10)){
		fprintf(stderr, "Error: could not setup audio monitor\n");
		if(debug_file)
			fclose(debug_file);
		return 1;
	}

	sigint_action.sa_handler = ctrl_c;
	sigint_action.sa_flags = 0;
	sigemptyset(&(sigint_action.sa_mask));
	sigaction(SIGINT, &sigint_action, NULL);

	sigemptyset(&block_sigint);
	sigaddset(&block_sigint, SIGINT);

	initscr();
	if(!has_colors()){
		endwin();
		fprintf(stderr, "Error: the terminal does not support colors\n");
		if(debug_file)
			fclose(debug_file);
		return 1;
	}
	pty_fd = open_terminal();
	if(pty_fd == -1 || fcntl(pty_fd, F_SETFL, O_NONBLOCK) < 0){
		endwin();
		fprintf(stderr, "Error: Could not make read-end of pipe non/blocking\n");
		if(debug_file)
			fclose(debug_file);
		return 1;
	}

	cbreak();
	start_color();
	if(COLORS >= 256){
		init_pair(1, COLOR_WHITE, COLOR_BLACK);
		init_pair(2, COLOR_WHITE, 52);
		init_pair(3, COLOR_WHITE, 58);
		init_pair(4, COLOR_WHITE, 22);
		red_background = 52;
		yellow_background = 58;
		green_background = 22;
	} else {
		init_pair(1, COLOR_WHITE, COLOR_BLACK);
		init_pair(2, COLOR_WHITE, COLOR_RED);
		init_pair(3, COLOR_WHITE, COLOR_YELLOW);
		init_pair(4, COLOR_WHITE, COLOR_GREEN);
		red_background = COLOR_RED;
		yellow_background = COLOR_YELLOW;
		green_background = COLOR_GREEN;
	}
	noecho();
	nodelay(stdscr, 1);
	scrollok(stdscr, 1);
	create_color_pairs(5);
	global_foreground_color = COLOR_WHITE;
	global_background_color = COLOR_BLACK;
	bkgd(get_global_color());
	erase();

	averages = calloc(COLS, sizeof(double));

	clock_gettime(CLOCK_MONOTONIC, &last_time);
	while(1){
		sigprocmask(SIG_SETMASK, &(sigint_action.sa_mask), NULL);
		while((key_press = getch()) != ERR){
			current_char = key_press;
			if(current_char == 0x7F)
				current_char = '\b';
			else if(current_char == '\n')
				current_char = '\r';
			if(debug_file)
				fprintf(debug_file, "INPUT: '%x', '%c'\n", current_char, current_char);
			if(write(pty_fd, &current_char, 1) < 0){
				fprintf(stderr, "Error: Failed to write to terminal device\n");
			}
		}
		FD_ZERO(&readable);
		FD_SET(pty_fd, &readable);
		no_wait.tv_sec = 0;
		no_wait.tv_usec = 0;

		if(select(pty_fd + 1, &readable, NULL, NULL, &no_wait) > 0){
			chars_read = read(pty_fd, buffer, 8191);
			if(chars_read > 0){
				print_bash_output(buffer);
				memset(buffer, 0, sizeof(char)*8192);
			} else if(chars_read <= 0){
				exit_terminal(0);
			}
		}
		//set_char_background(0, 0, color_pairs_green);
		curs_set(0);
		update_visualizer();
		curs_set(1);
		refresh();
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		last_nanoseconds = get_nanoseconds(last_time);
		current_nanoseconds = get_nanoseconds(current_time);
		if(current_nanoseconds - last_nanoseconds < 25000000ULL){
			sleep_time = (struct timespec) {.tv_sec = 0, .tv_nsec = 25000000ULL - current_nanoseconds + last_nanoseconds};
			sigprocmask(SIG_SETMASK, &block_sigint, NULL);
			nanosleep(&sleep_time, NULL);
			sigprocmask(SIG_SETMASK, &(sigint_action.sa_mask), NULL);
			if(do_ctrl_c){
				write(pty_fd, "\x03", 1);
				do_ctrl_c = 0;
			}
			last_time.tv_sec = (last_nanoseconds + 25000000)/1000000000ULL;
			last_time.tv_nsec = (last_nanoseconds + 25000000)%1000000000ULL;
		} else {
			clock_gettime(CLOCK_MONOTONIC, &last_time);
		}
	}
}
