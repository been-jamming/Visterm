#include <SDL2/SDL.h>
#include <complex.h>

extern SDL_AudioDeviceID recording_device_id;
extern complex double *frequencies;
extern float *samples;

int audio_monitor_setup(unsigned int freq, unsigned int log_samp);

