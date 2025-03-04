struct temp_block * region_blocks;
struct region_header { int processed; int length; int l; int bx,by,ex,ey; };
signed char * blend_add_region(int layer, int bx, int by, int ex, int ey);
void blend_palette(int amount, unsigned int * colors);
void blend_setup();
