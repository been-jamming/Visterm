//Used for reverse lookup of color pairs

#include <stdlib.h>
#include "hollow_list.h"

static int get_num_zeros(unsigned int n, unsigned int upper_bound){
	int output = 0;

	while(upper_bound){
		if(!(n&1)){
			output++;
			n >>= 1;
			upper_bound--;
		} else {
			return output;
		}
	}

	return output;
}

//size can be up to the number of bits in an unsigned int
hollow_list *create_hollow_list(int size, int value){
	hollow_list *output;

	output = malloc(sizeof(hollow_list));
	if(!output)
		return NULL;
	output->size = size;
	output->children = calloc(size, sizeof(hollow_list *));
	if(!output->children){
		free(output);
		return NULL;
	}
	output->value = value;

	return output;
}

void free_hollow_list(hollow_list *list, void (*free_value)(int)){
	int i;

	free_value(list->value);

	for(i = 0; i < list->size; i++){
		if(list->children[i]){
			free_hollow_list(list->children[i], free_value);
		}
	}
	free(list->children);
}

int read_hollow_list(hollow_list *list, unsigned int n, int default_value){
	int num_zeros;

	if(!list)
		return default_value;
	while(n){
		num_zeros = get_num_zeros(n, list->size - 1);
		n >>= num_zeros + 1;
		list = list->children[num_zeros];
		if(!list)
			return default_value;
	}

	return list->value;
}

int write_hollow_list(hollow_list *list, unsigned int n, int value, int default_value){
	int num_zeros;
	hollow_list *new_list;

	if(!list)
		return 1;

	while(n){
		num_zeros = get_num_zeros(n, list->size - 1);
		n >>= num_zeros + 1;
		new_list = list->children[num_zeros];
		if(!new_list){
			new_list = create_hollow_list(list->size - num_zeros - 1, default_value);
			list->children[num_zeros] = new_list;
		}
		if(!new_list)
			return 1;
		list = new_list;
	}

	list->value = value;

	return 0;
}

