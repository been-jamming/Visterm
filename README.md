# Visterm
A terminal application which adds an audio visualizer to the terminal!

## Dependencies
 - ncurses
 - SDL2

To build, use `make`, which outputs a binary named `visterm.out`

In order to use the terminal, install by running `make install` and run with `visterm` in a curses compatible terminal. If you don't want to install, run `tic src/visterm.termcap` and run `./visterm.out`. `tic` is the terminfo compiler, which will install a terminfo file describing the capabilities of visterm to other console programs.

`visterm` can be run inside of itself! It is compatible with most applications which use console colors, including VIM. If you find bugs, feel free to open an issue.
