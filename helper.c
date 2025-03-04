# include <assert.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>
# include "helper.h"

/*block size*/
static const int BSIZE = 4096 + sizeof(struct temp_block);

void mem_request(struct temp_block ** cur_p, int size) {
	struct temp_block * cur;
	struct temp_block * new;

	assert(size<=BSIZE);
	cur = * cur_p;
	if (!cur->head) {
		cur = calloc(1, BSIZE + 0x80);
		assert(cur);
		cur->data = (unsigned char *)cur +sizeof(struct temp_block);
		cur->end = cur->data +size;
		cur->head = cur;
		* cur_p = cur;
	} else if ((cur->end +size) > ((unsigned char *)cur +BSIZE)) {
		if (! cur->next) {
			new = calloc(1, BSIZE);
			new->data = (unsigned char *)new +sizeof(struct temp_block);
			new->end = new->data +size;
			new->head = cur->head;
			cur->next = new;
			* cur_p = new;
		} else {
			new = cur->next;
			new->data = (unsigned char *)new +sizeof(struct temp_block);
			new->end = new->data +size;
			* cur_p = new;
		}
/*
		new = calloc(1, BSIZE +0x80);
		assert(new);
		new->data = (unsigned char *)new +sizeof(struct temp_block);
		new->end = new->data +size;
		new->head = cur->head;
		cur->next = new;
		* cur_p = new;
*/
	} else {
		cur->data = cur->end;
		cur->end += size;
	}
}

int mem_loop(int init, struct temp_block ** cur_p) {
	struct temp_block * cur;
	struct temp_block * loop;
	
	cur = * cur_p;
	if (! cur->head)
		return 0;
	if (init) {
		cur = cur->head;
		cur->data = (unsigned char *)cur +sizeof(struct temp_block);
		* cur_p = cur;
		return 1;
	}
	if (cur->data >= cur->end) {
		if (! cur->next)
			return 0;
		cur = cur->next;
		cur->data = (unsigned char *)cur +sizeof(struct temp_block);
		* cur_p = cur;
	}
	return 1;
}

void mem_free(struct temp_block ** cur_p) {
	struct temp_block * cur;
	struct temp_block * temp_next;
	
	cur = * cur_p;
	if (! cur->head)
		return;
	for (cur=cur->head; cur; cur=temp_next) {
		temp_next = cur->next;
		free(cur);
	}
	* cur_p = & empty_block;
}

/*for persistent or frame-by-frame data*/
/*no free, but skeleton of linked list stays*/
/*TODO:memset?*/
void mem_reset(struct temp_block ** cur_p) {
	struct temp_block * cur;

	cur = * cur_p;
	if (! cur->head)
		return;
	for (cur=cur->head; cur; cur=cur->next) {
		cur->data = (unsigned char *)cur +sizeof(struct temp_block);
		cur->end = cur->data;
	}
	cur = * cur_p;
	cur = cur->head;
	* cur_p = cur;
}

unsigned char * mem_finish(unsigned char * data, int * old_size, struct temp_block ** cur_p) {
	int new_size;
	int temp_size;
	struct temp_block * cur;
	struct temp_block * loop;
	struct temp_block * temp_next;
	unsigned char * data_copy;

	cur = * cur_p;
	new_size = 0;
	for (loop=cur->head; loop; loop=loop->next) {
		loop->data = (unsigned char *)loop +sizeof(struct temp_block);
		new_size += loop->end - loop->data;
	}
	data = realloc(data, new_size + * old_size);
	data_copy = data + * old_size;
	for (loop=cur->head; loop; loop=temp_next) {
		temp_next = loop->next;
		temp_size = loop->end - loop->data;
		memcpy(data_copy, loop->data, temp_size);
		free(loop);
		data_copy += temp_size;
	}
	* old_size += new_size;
	* cur_p = & empty_block;
	return data;
}

int midmod(int start,int end,int align) {
	int mid;

	mid=start+(end-start)/2;
	mid-=mid%align;
	return mid;
}
