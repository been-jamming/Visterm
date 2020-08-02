default:
	gcc audio_monitor.c fft.c open_terminal.c term.c -lncurses -lSDL2 -lm -o visterm
