void draw_rectangle(int layer, int bx, int by, int ex, int ey, signed char color) {
	static signed char * buffer;
	int w,h;
	int i;

	w = ex-bx;
	h = ey-by;
	if (w*h>1024) {
		if (w>1024) {
			msg("rectangle too large.");
			ex=bx+1024;
		}
		for (i=0;i<h;i++)
			draw_rectangle(layer,bx,by+i,ex,by+i+1,color);
		return;
	}
	buffer = blend_add_region(layer,bx,by,ex,ey);
	memset(buffer,color,w*h);
}
