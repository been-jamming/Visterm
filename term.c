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

int open_terminal();
double *averages;
int do_ctrl_c = 0;

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
	while(*str){
		if(*str == 0x1B && str[1] == 'c'){
			str++;
			erase();
		} else if(*str == '\a')
			fputc('\a', stdout);
		else if(*str != '\r')
			printw("%c", (int) *str);
		str++;
	}
}

void exit_terminal(){
	refresh();
	endwin();
	SDL_Quit();
	close(pty_fd);
	free(frequencies);
	free(samples);
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

void draw_bar(int x, int y, int term_width){
	if(x > term_width - 1)
		x = term_width - 1;
	mvchgat(y, 0, -1, A_NORMAL, COLOR_BLACK, NULL);
	/*
	if(x > (term_width - 1)*2/3){
		mvchgat(y, term_width - 1 - x, -1, A_NORMAL, 2, NULL);
		mvchgat(y, (term_width - 1)/3, -1, A_NORMAL, 3, NULL);
		mvchgat(y, (term_width - 1)*2/3, -1, A_NORMAL, 4, NULL);
	} else if(x > (term_width - 1)/5){
		mvchgat(y, term_width - 1 - x, -1, A_NORMAL, 3, NULL);
		mvchgat(y, (term_width - 1)*4/5, -1, A_NORMAL, 4, NULL);
	} else {
		mvchgat(y, term_width - 1 - x, -1, A_NORMAL, 4, NULL);
	}
	*/
	mvchgat(y, term_width - 1 - x, -1, A_NORMAL, 2, NULL);
	mvchgat(y, term_width - 1 - x*2/3, -1, A_NORMAL, 3, NULL);
	mvchgat(y, term_width - 1 - x/3, -1, A_NORMAL, 4, NULL);
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
}

int main(int argc, char **argv){
	char buffer[256] = {0};
	char in_buffer[256] = {0};
	unsigned char current_pos = 0;
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

	if(audio_monitor_setup(44100/4, 10)){
		fprintf(stderr, "Error: could not setup audio monitor\n");
		exit(1);
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
		return 1;
	}
	pty_fd = open_terminal();
	if(pty_fd == -1 || fcntl(pty_fd, F_SETFL, O_NONBLOCK) < 0){
		endwin();
		fprintf(stderr, "Error: Could not make read-end of pipe non/blocking\n");
		return 1;
	}

	cbreak();
	start_color();
	if(COLORS >= 256){
		init_pair(1, COLOR_WHITE, COLOR_BLACK);
		init_pair(2, COLOR_WHITE, 52);
		init_pair(3, COLOR_WHITE, 58);
		init_pair(4, COLOR_WHITE, 22);
	} else {
		init_pair(1, COLOR_WHITE, COLOR_BLACK);
		init_pair(2, COLOR_WHITE, COLOR_RED);
		init_pair(3, COLOR_WHITE, COLOR_YELLOW);
		init_pair(4, COLOR_WHITE, COLOR_GREEN);
	}
	bkgd(COLOR_PAIR(1));
	noecho();
	nodelay(stdscr, 1);
	scrollok(stdscr, 1);
	attron(COLOR_PAIR(1));
	erase();

	averages = calloc(COLS, sizeof(double));

	clock_gettime(CLOCK_MONOTONIC, &last_time);
	while(1){
		sigprocmask(SIG_SETMASK, &(sigint_action.sa_mask), NULL);
		while((key_press = getch()) != ERR){
			current_char = key_press;
			if(current_char == 0x7F)
				current_char = '\b';
			if(write(pty_fd, &current_char, 1) < 0){
				fprintf(stderr, "Error: Failed to write to terminal device\n");
			}
		}
		FD_ZERO(&readable);
		FD_SET(pty_fd, &readable);
		no_wait.tv_sec = 0;
		no_wait.tv_usec = 0;

		if(select(pty_fd + 1, &readable, NULL, NULL, &no_wait) > 0){
			chars_read = read(pty_fd, buffer, 255);
			if(chars_read > 0){
				print_bash_output(buffer);
				memset(buffer, 0, sizeof(char)*256);
			} else if(chars_read <= 0){
				exit_terminal(0);
			}
		}
		update_visualizer();
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

