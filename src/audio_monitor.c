#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include <complex.h>

static unsigned int frequency;
static unsigned int num_samples_log;

SDL_AudioDeviceID recording_device_id;
complex double *frequencies;
float *samples;

void recording_callback(void *user_data, uint8_t *stream, int len){
	memcpy(samples, stream, sizeof(float)*(1<<num_samples_log));
}

int audio_monitor_setup(unsigned int freq, unsigned int log_samp){
	SDL_AudioSpec desired_spec;
	SDL_AudioSpec output_spec;
	int num_output_devices;
	int i;

	frequency = freq;
	num_samples_log = log_samp;

	if(!SDL_SetHint(SDL_HINT_AUDIO_INCLUDE_MONITORS, "1")){
		fprintf(stderr, "Failed to set hint\n");
		exit(1);
	}
	if(SDL_Init(SDL_INIT_AUDIO) < 0)
		return 1;

	num_output_devices = SDL_GetNumAudioDevices(SDL_TRUE);
	if(!num_output_devices){
		SDL_Quit();
		return 1;
	}

	for(i = 0; i < num_output_devices; i++){
		printf("%d. %s\n", i, SDL_GetAudioDeviceName(i, 1));
	}
	printf("Enter choice: ");
	scanf("%d", &i);

	SDL_zero(desired_spec);
	desired_spec.freq = frequency;
	desired_spec.format = AUDIO_F32SYS;
	desired_spec.channels = 1;
	desired_spec.samples = 1<<log_samp;
	desired_spec.callback = recording_callback;
	frequencies = calloc(1<<log_samp, sizeof(complex double));
	samples = calloc(1<<log_samp, sizeof(float));

	recording_device_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(i, 1), SDL_TRUE, &desired_spec, &output_spec, 0);
	SDL_PauseAudioDevice(recording_device_id, SDL_FALSE);

	return 0;
}

