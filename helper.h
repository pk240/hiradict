struct temp_block {
	struct temp_block * head;
	struct temp_block * next;
	unsigned char * data;
	unsigned char * end;
};
struct temp_block empty_block;
void mem_request(struct temp_block ** cur_p, int size);
int mem_loop(int init, struct temp_block ** cur_p);
void mem_free(struct temp_block ** cur_p);
unsigned char * mem_finish(unsigned char * data, int * old_size, struct temp_block ** cur_p);
