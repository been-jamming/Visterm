# default:
# 	gcc audio_monitor.c fft.c open_terminal.c term.c -lncurses -lSDL2 -lm -o visterm

	
CC	:= gcc
LFLAGS	:= -lSDL2 -lm 
CFLAGS	:= -Wall #-Werror 

RM = rm -f   # rm command
PROJECT_NAME = visterm 
TARGET_BIN = visterm.out  
BIN_LOC = /usr/local

SRCS = src/audio_monitor.c src/fft.c src/open_terminal.c src/term.c 

OBJS = $(SRCS:.c=.o)



all: ${TARGET_BIN}


$(TARGET_BIN): $(OBJS)
	$(CC) ${CFLAGS} -o $@ $^ ${LFLAGS}

$(OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ ${LFLAGS}

install: ${TARGET_BIN}
	cp ${TARGET_BIN} ${PROJECT_NAME}
	install -m 0755 $(PROJECT_NAME) $(BIN_LOC)/bin
	rm $(PROJECT_NAME)



clean:
	-${RM} ${TARGET_BIN} ${OBJS}
