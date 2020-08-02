#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include "fft.h"

static unsigned int frequency;
static unsigned int num_samples_log;

SDL_AudioDeviceID recording_device_id;
complex double *frequencies;
static complex double *samples;

static SDL_AudioSpec output_spec;

void recording_callback(void *user_data, uint8_t *stream, int len){
	int i;

	for(i = 0; i < 1<<num_samples_log; i++){
		samples[i] = ((int16_t) stream[i*2]) | ((int16_t) (stream[i*2 + 1]<<8));
		samples[i] /= 65536/2;
	}

	dfft2(frequencies, samples, num_samples_log);
}

int audio_monitor_setup(unsigned int freq, unsigned int log_samp){
	SDL_AudioSpec desired_spec;
	SDL_AudioSpec output_spec;
	int num_output_devices;

	frequency = freq;
	num_samples_log = log_samp;

	if(SDL_Init(SDL_INIT_AUDIO) < 0)
		return 1;

	num_output_devices = SDL_GetNumAudioDevices(SDL_TRUE);
	if(!num_output_devices){
		SDL_Quit();
		return 1;
	}

	SDL_zero(desired_spec);
	desired_spec.freq = frequency;
	desired_spec.format = AUDIO_S16LSB;
	desired_spec.channels = 1;
	desired_spec.samples = 1<<log_samp;
	desired_spec.callback = recording_callback;
	frequencies = calloc(1<<log_samp, sizeof(complex double));
	samples = calloc(1<<log_samp, sizeof(complex double));

	recording_device_id = SDL_OpenAudioDevice(NULL, SDL_TRUE, &desired_spec, &output_spec, 0);
	SDL_PauseAudioDevice(recording_device_id, SDL_FALSE);

	return 0;
}

