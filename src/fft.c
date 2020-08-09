//60Hz to 4kHz
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <complex.h>
#include <math.h>

static uint16_t reverse_uint16(uint16_t n, int num_bits){
	n = ((n&0x00FF)<<8) | ((n&0xFF00)>>8);
	n = ((n&0x0F0F)<<4) | ((n&0xF0F0)>>4);
	n = ((n&0x3333)<<2) | ((n&0xCCCC)>>2);
	n = ((n&0x5555)<<1) | ((n&0xAAAA)>>1);

	n >>= 16 - num_bits;
	return n;
}

static void reverse_list(complex double *list0, complex double *list1, int log_size){
	uint16_t i;

	for(i = 0; i < 1<<log_size; i++)
		list0[reverse_uint16(i, log_size)] = list1[i];
}

static void reverse_list_float(complex double *list0, float *list1, int log_size){
	uint16_t i;

	for(i = 0; i < 1<<log_size; i++)
		list0[reverse_uint16(i, log_size)] = list1[i];
}

void dfft2(complex double *output, complex double *input, int log_size){
	uint16_t size;
	int s, m, k, j;
	complex double w, wm, t, u;

	reverse_list(output, input, log_size);
	size = 1<<log_size;
	for(s = 1; s <= log_size; s++){
		m = 1<<s;
		wm = cexp(-2*M_PI*I/m);
		for(k = 0; k < size; k += m){
			w = 1;
			for(j = 0; j < m/2; j++){
				t = w*output[k + j + m/2];
				u = output[k + j];
				output[k + j] = u + t;
				output[k + j + m/2] = u - t;
				w = w*wm;
			}
		}
	}
}

void dfft2_float(complex double *output, float *input, int log_size){
	uint16_t size;
	int s, m, k, j;
	complex double w, wm, t, u;

	reverse_list_float(output, input, log_size);
	size = 1<<log_size;
	for(s = 1; s <= log_size; s++){
		m = 1<<s;
		wm = cexp(-2*M_PI*I/m);
		for(k = 0; k < size; k += m){
			w = 1;
			for(j = 0; j < m/2; j++){
				t = w*output[k + j + m/2];
				u = output[k + j];
				output[k + j] = u + t;
				output[k + j + m/2] = u - t;
				w = w*wm;
			}
		}
	}
}
