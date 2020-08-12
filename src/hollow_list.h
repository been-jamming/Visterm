typedef struct hollow_list hollow_list;

struct hollow_list{
	int size;
	hollow_list **children;
	int value;
};

hollow_list *create_hollow_list(int size, int value);
void free_hollow_list(hollow_list *list, void (*free_value)(int));
int read_hollow_list(hollow_list *list, unsigned int n, int default_value);
int write_hollow_list(hollow_list *list, unsigned int n, int value, int default_value);

