# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# define MAXBM 2048

static int blackband_x, blackband_y;
static int resizing;
static int integer_scaling;
static int paint_bb;

static short tru2hi(int tru) {
	//BYTE a;
	BYTE r,g,b;
	short c;

	//a=(tru&0xff000000)>>24;
	r=(tru&0x00f80000)>>16; // 5 most significant bits RRRRR000
	g=(tru&0x0000fc00)>>8; // 6 bits GGGGGG00
	b=(tru&0x000000f8); // 5 bits BBBBB000

	//RRRRR000<<<<<<<<     8<
	//rrrrrGGGGGG00<<<     3<
	//rrrrrggggggBBBBB>>> -3>
	c=r<<8;
	c|=g<<3;
	c|=b>>3;
	return c;
}

static void game_to_window(HDC hdc, int depth, float scaleX,float scaleY, int bb_x,int bb_y, int sbx,int sby,int sex,int sey) {
	static unsigned int window_bitmap[MAXBM]; //space for a sprite or a scanline
	int sw,sh, gw,gh; //sprite size and game bitmap size
	int x,y;
	int gameX,gameY; //game bitmap coords

	sw=sex-sbx;
	sh=sey-sby;
	gw=game_bitmap_w;
	gh=game_bitmap_h;
	if (sw *sh > MAXBM) {
		if (sw > MAXBM) {
			sw = MAXBM;
			sex = sbx +MAXBM;
			msg("Screen too large.");
		}
		for (y = 0; y < sh; y++)
			game_to_window(hdc,depth,scaleX,scaleY, bb_x,bb_y, sbx,sby+y, sex,sby+y+1);
		return;
	}
	for (y = 0; y < sh; y++) {
		gameY = (int)((y +sby -bb_y) / scaleY);
		if (gameY >= 0 && gameY < gh) {
			for (x = 0; x < sw; x++) {
				gameX = (int)((x +sbx -bb_x) / scaleX);
				if (gameX >= 0 && gameX < gw) {
					if (depth==16)
						((short *)window_bitmap)[y * sw + x] = tru2hi(game_bitmap[gameY * gw + gameX]);
					else if (depth==32)
						window_bitmap[y * sw + x] = game_bitmap[gameY * gw + gameX];
				} else {
					if (depth==16)
						((short *)window_bitmap[y * sw + x]) = 0;
					else if (depth==32)
						window_bitmap[y * sw + x] = 0;
				}
			}
		} else {
			for (x = 0; x < sw; x++) {
				if (depth==16)
					((short *)window_bitmap[y * sw + x]) = 0;
				else if (depth==32)
					window_bitmap[y * sw + x] = 0;
			}
		}
	}
	HBITMAP pbb_small = CreateBitmap(sw,sh, 1,depth,0);
	SetBitmapBits(pbb_small, depth/8 *sw *sh, window_bitmap);
	HDC hdcMem = (HBITMAP)CreateCompatibleDC(hdc);
	HBITMAP hbmOld = SelectObject(hdcMem, pbb_small);
	BitBlt(hdc, sbx,sby, sex,sey, hdcMem, 0,0, SRCCOPY);
	SelectObject(hdcMem, hbmOld);
	DeleteObject(hdcMem);
	DeleteObject(hbmOld);
	DeleteObject(pbb_small);
}

static void recalculate_blackbands() {
	int fx,fy,factor,tx,ty;
	float rx,ry;
	int gw,gh;
	int width,height;

	width=window_w; height=window_h;
	gw=game_bitmap_w; gh=game_bitmap_h;
	if (integer_scaling && width > gw && height > gh) {
		fx = width/gw;
		fy = height/gh;
		factor = fx>fy?fy:fx;
		tx = gw * factor;
		ty = gh * factor;
		msg("target: %dx%d", tx, ty);
		blackband_x = (width - tx) /2;
		blackband_y = (height - ty) /2;
	} else {
		rx = (float)width/gw;
		ry = (float)height/gh;
		if (rx>ry) {
			blackband_x = (width - (ry * gw)) /2;
			blackband_y = 0;
		} else if (ry>rx) {
			blackband_x = 0;
			blackband_y = (height - (rx * gh)) /2;
		} else {
			blackband_x = 0;
			blackband_y = 0;
		}
	}
}

static int blit_create(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	return 0;
}

static int blit_paint(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;
	float scaleX, scaleY;
	struct region_header * reg;
	int depth;

	if (resizing) return 0;
	hdc = BeginPaint(hwnd, & ps);
	depth=GetDeviceCaps(hdc, BITSPIXEL);
	msg("Depth:%d",depth);
	scaleX = (float)(window_w -blackband_x *2)/game_bitmap_w;
	scaleY = (float)(window_h -blackband_y *2)/game_bitmap_h;
	if (paint_bb || !region_blocks || region_blocks==& empty_block) {
		paint_bb=0;
		if (region_blocks==& empty_block)
			msg("Taint repaint");
		else
			msg("Full repaint in paint, %dx%d", blackband_x, blackband_y);
		game_to_window(hdc,depth,scaleX,scaleY,blackband_x,blackband_y,0,0,window_w,window_h);
	} else {
		mem_loop(1, & region_blocks);
		while (mem_loop(0, & region_blocks)) {
			reg = (struct region_header *)region_blocks->data;
			region_blocks->data += sizeof(struct region_header);
			if (integer_scaling) scaleX = (int)scaleX, scaleY = (int)scaleY;
			int screen_bx = blackband_x + reg->bx * scaleX;
			if (screen_bx<0) screen_bx=0;
			int screen_ex = blackband_x + reg->ex * scaleX;
			if (screen_ex>window_w) screen_ex=window_w;
			int screen_by = blackband_y + reg->by * scaleY;
			if (screen_by<0) screen_by=0;
			int screen_ey = blackband_y + reg->ey * scaleY;
			if (screen_ey>window_h) screen_ey=window_h;
			game_to_window(
				hdc,
				depth,
				scaleX, scaleY,
				blackband_x, blackband_y,
				screen_bx, screen_by,
				screen_ex, screen_ey
			);
			region_blocks->data += reg->length *sizeof(unsigned int);
		}
	}
	EndPaint(hwnd, & ps);
	return 0;
}

static int blit_debug(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	int x,y;
	char * m;
	int p,q;

	if (! ((lParam>>30) %2)) {
		if (wParam == VK_F1) {
			write_debug=1;
			msg("DEBUG F1");
			msg("Filling %dx%d",game_bitmap_w,game_bitmap_h);
			for (y=0;y<game_bitmap_h;y++) for (x=0;x<game_bitmap_w;x++) {
				//game_bitmap[y*game_bitmap_w+x]=0x007777ff;
				if (y%90<30)
				game_bitmap[y*game_bitmap_w+x]=0x000000ff;
				else if (y%90<60)
				game_bitmap[y*game_bitmap_w+x]=0x0000ff00;
				else
				game_bitmap[y*game_bitmap_w+x]=0x00ff0000;
			}
/*
			m = msg(0);
			if (m) {
				for (p=-1;p<2;p++) for (q=-1;q<2;q++)
					blit32_TextExplicit(game_bitmap, 0x00ffff77,1,game_bitmap_w,game_bitmap_h,0,10+p,10+q,m);
				blit32_TextExplicit(game_bitmap, 0,1,game_bitmap_w,game_bitmap_h,0,10,10,m);
				free(m);
			}
*/
			InvalidateRect(hwnd, NULL, TRUE);
			paint_bb=1; //skip blend
		} else if (wParam == VK_F3) {
			integer_scaling=!integer_scaling;
			recalculate_blackbands();
			paint_bb=1;
			InvalidateRect(hwnd, NULL, TRUE);
		}
	}
	return 0;
}

static int blit_size(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	int new_w = LOWORD(lParam);
	int new_h = HIWORD(lParam);
	if (new_w>1000||new_h>1000||new_w<100||new_h<100) {
		msg("Weird resize: %dx%d. ignoring\r\n",new_w,new_h);
		return 0;
	}
	window_w = new_w;
	window_h = new_h;
	if (!resizing) {
		msg("New size without resize: %dx%d",window_w,window_h);
		recalculate_blackbands();
		paint_bb=1;
		InvalidateRect(hwnd, NULL, TRUE);
	}
	return 0;
}

static int blit_entersizemove(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	resizing = 1;
	return 0;
}

static int blit_exitsizemove(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	static int oldw,oldh;

	resizing = 0;
	recalculate_blackbands();
	paint_bb=1;
	msg("resize:%dx%d -> %dx%d",oldw,oldh,window_w,window_h);
	oldw=window_w;oldh=window_h;
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;
}

static int blit_repaint(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	float scaleX,scaleY;
	RECT rrr;
	struct region_header * reg;

	if ((enum flags)wParam!=F_REPAINT)
		return;
	scaleX = (float)(window_w -blackband_x *2)/game_bitmap_w;
	scaleY = (float)(window_h -blackband_y *2)/game_bitmap_h;
	if (integer_scaling) {
		scaleX = (int)scaleX; scaleY = (int)scaleY;
	}
	mem_loop(1, & region_blocks);
	while (mem_loop(0, & region_blocks)) {
		reg = (struct region_header *)region_blocks->data;
		region_blocks->data += sizeof(struct region_header);
		int screen_bx = blackband_x + reg->bx * scaleX;
		int screen_by = blackband_y + reg->by * scaleY;
		int screen_ex = blackband_x + reg->ex * scaleX;
		int screen_ey = blackband_y + reg->ey * scaleY;
		rrr=(RECT){screen_bx,screen_by, screen_ex,screen_ey};
		InvalidateRect(hwnd, &rrr, FALSE);
		region_blocks->data += reg->length *sizeof(unsigned int);
	}
	
}

void blit_setup() {
	game_bitmap_w = 200;
	game_bitmap_h = 400;
	game_bitmap = malloc(sizeof(unsigned int) *game_bitmap_w *game_bitmap_h);
	window_w = 220;
	window_h = 500;
	integer_scaling = 1;
	//add_handler(H_CREATE, & blit_create);
	add_handler(H_PAINT_LO, & blit_paint);
	add_handler(H_ENTERSIZEMOVE, & blit_entersizemove);
	add_handler(H_EXITSIZEMOVE, & blit_exitsizemove);
	add_handler(H_SIZE, & blit_size);
	add_handler(H_KEY, & blit_debug);
	add_handler(H_FLAG, & blit_repaint);
}
