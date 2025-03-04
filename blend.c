# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
static int layer_count;
static int layer_shadow;
static signed char ** layers;
static int layers_w,layers_h;
static unsigned int * current_palette;
static unsigned int * blend_array;

static void blend_resize_layers() {
	int i;

	if (layers_w==game_bitmap_w && layers_h==game_bitmap_h)
		return;
	msg("Layer resize. %dx%d -> %dx%d",layers_w,layers_h,game_bitmap_w,game_bitmap_h);
	layers_w = game_bitmap_w;
	layers_h = game_bitmap_h;
	//for (i=0; i<L_BACKUP; i++) {
	for (i=0; i<layer_count; i++) {
		layers[i] = realloc(layers[i], game_bitmap_w *game_bitmap_h);
		memset(layers[i], -1, game_bitmap_w *game_bitmap_h);
	}
	msg("new layers size: %dx%d", layers_w,layers_h);
}

static unsigned int blend_mix(unsigned int l1, unsigned int l2) {
	unsigned int r1,r2, g1,g2, b1,b2;

	r1=(l1 &0x00ff0000) >>16;
	r2=(l2 &0x00ff0000) >>16;
	g1=(l1 &0x0000ff00) >>8;
	g2=(l2 &0x0000ff00) >>8;
	b1=(l1 &0x000000ff);
	b2=(l2 &0x000000ff);
	return 0xff000000 |
		((r1 +r2) /2 <<16) |
		((g1 +g2) /2 <<8 ) |
		((b1 +b2) /2);
}

void blend_palette(int amount, unsigned int * colors) {
	int i;

	current_palette = realloc(current_palette, sizeof(unsigned int) *amount);
	memcpy(current_palette, colors, sizeof(unsigned int) *amount);
}

signed char * blend_add_region(int layer, int bx, int by, int ex, int ey) {
	struct region_header new;
	int length;

	assert(layer<layer_count);
	length = (ex -bx) *(ey -by);
	mem_request(& region_blocks, sizeof(struct region_header) +length *sizeof(unsigned int));
	new = (struct region_header) {
		.length=length, .l=layer, .bx=bx,.by=by, .ex=ex,.ey=ey,
	};
	memcpy(region_blocks->data, & new, sizeof(struct region_header));
	return region_blocks->data +sizeof(struct region_header);
}

static void blend_free_regions() {
	mem_free(& region_blocks);
}

static void blend_write_regions() {
	struct region_header * reg;
	signed char * sconv;
	int x,y,i;

	mem_loop(1, & region_blocks);
	while (mem_loop(0, & region_blocks)) {
		i=0;
		reg = (struct region_header *)region_blocks->data;
		region_blocks->data += sizeof(struct region_header);
		sconv = region_blocks->data;
		for (y=reg->by;y<reg->ey;y++) for (x=reg->bx;x<reg->ex;x++) {
			layers[reg->l][y*game_bitmap_w+x] = sconv[i++];
		}
		region_blocks->data += reg->length *sizeof(unsigned int);
	}
}

static void blend() {
	int y,x,l;
	int shadow, idx;
	unsigned int c;
	struct region_header * reg;

	mem_loop(1, & region_blocks);
	while (mem_loop(0, & region_blocks)) {
		reg = (struct region_header *)region_blocks->data;
		region_blocks->data += sizeof(struct region_header);
		for (y=reg->by;y<reg->ey;y++) for (x=reg->bx;x<reg->ex;x++) {
			c=0;
			shadow=-1;
			//for (l=L_BACKUP -1; l>=0; l--) {
			for (l=layer_count -1; l>=0; l--) {
				idx=layers[l][y*game_bitmap_w+x];
				if (idx==-1) continue;
				//if (l==L_SHADOW) {shadow=current_palette[idx]; continue;}
				if (l==layer_shadow) {shadow=current_palette[idx]; continue;}
				c=current_palette[idx]; break;
			}
			if (shadow!=-1) c=blend_mix(c,(unsigned int)shadow);
			game_bitmap[y*game_bitmap_w+x]=0xff000000 |c;
		}
		region_blocks->data += reg->length *sizeof(unsigned int);
	}
}

static int blend_test(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	static unsigned int palette[10];
	static signed char * buffer;
	int i;

	time_t t;
	srand((unsigned) time(&t));
	if (! ((lParam>>30) %2)) {
		if (wParam == VK_F2) {
			msg("in F2");
			for (i=0;i<10;i++) palette[i]=rand();
			blend_palette(10,palette);
			buffer = blend_add_region(3,40,40,60,70);
			for (i=0;i<600;i++) buffer[i]=(rand()%11)-1;
			return F_REPAINT;
		}
	}
	return F_NONE;
}

void blend_setup(int lcount,int lshadow) {
	layer_count=lcount;
	layer_shadow=lshadow;
	layers=calloc(1,sizeof(signed char *) *layer_count);
	blend_array=malloc(sizeof(signed int) *layer_count);
	region_blocks = & empty_block;
	add_handler(H_KEY, & blend_test);
	add_handler(H_PAINT_HI, & blend_resize_layers);
	add_handler(H_PAINT_HI, & blend_write_regions);
	add_handler(H_PAINT_HI, & blend);
	add_handler(H_PAINT_LO, & blend_free_regions);
}
