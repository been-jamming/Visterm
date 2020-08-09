#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <sys/ioctl.h>
#include <ncurses.h>

extern char **environ;

int open_terminal(){
	int master;
	int slave;
	char *slave_name;
	pid_t p;
	char * const argv2[] = {"/bin/bash", 0};
	char path_buffer[1024];
	char home_buffer[1024];
	char *path;
	char *home;
	char *env[] = {"TERM=dumb", "COLORTERM=", 0, 0, 0};
	struct winsize size;

	path = getenv("PATH");
	if(path){
		snprintf(path_buffer, 1024, "PATH=%s", path);
		env[2] = path_buffer;
	}
	home = getenv("HOME");
	if(home){
		snprintf(home_buffer, 1024, "HOME=%s", home);
		env[3] = home_buffer;
	}

	master = posix_openpt(O_RDWR | O_NOCTTY);
	if(master < 0){
		return -1;
	}
	if(grantpt(master) < 0){
		close(master);
		return -1;
	}
	if(unlockpt(master) < 0){
		close(master);
		return -1;
	}
	slave_name = ptsname(master);
	if(!slave_name){
		close(master);
		return -1;
	}
	slave = open(slave_name, O_RDWR | O_NOCTTY);
	if(slave < 0){
		close(master);
		return -1;
	}

	p = fork();
	if(!p){
		close(master);
		setsid();
		if(ioctl(slave, TIOCSCTTY, NULL) < 0){
			close(slave);
			fprintf(stderr, "Error: ioctl failed\n");
			exit(1);
		}
		size.ws_row = LINES;
		size.ws_col = COLS;
		size.ws_xpixel = 0;
		size.ws_ypixel = 0;
		if(ioctl(slave, TIOCSWINSZ, &size) < 0){
			close(slave);
			fprintf(stderr, "Error: ioctl failed\n");
			exit(1);
		}
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		close(slave);
		execve("/bin/bash", argv2, env);

		return -1;//Only to prevent a warning
	} else if(p > 0){
		close(slave);
		return master;
	} else {
		return -1;
	}
}

