int issjis(BYTE c) {
	return c>0x7E && c<0xA6 || c>0xDF;
}

static unsigned short readfont(int line,int bytes,int pos_x,int pos_y) {
	static FILE * f;
	static unsigned int offset=0x3E; //length of headers in bmp
	static unsigned int anex_w=2048/8; //amount of BYTES on each line
	unsigned int pixel_pos;
	unsigned short value;

	if (!f) f=fopen("anex86.bmp","rb");
	if (!f) msg("Error opening font\n");
	pixel_pos=(line+pos_y*16)*anex_w+pos_x*bytes;
	fseek(f,offset+pixel_pos,SEEK_SET);
	if (bytes==2) {
		if (!fread(& value,1,1,f)) msg("error readfont\n");
		value<<=8;
	}
	if (!fread(& value,1,1,f)) msg("error readfont\n");
	return value;
}

static void drawbytes(signed char * bufline,unsigned short fontbytes,int amount) {
	int i;

	for (i=0;i<amount*8;i++) {
		//if (! (fontbytes&1)) bitmap_draw(x+amount*8-i,y);
		* (bufline+(amount*8-i-1))=(fontbytes&1) ? -1 : 1;
		fontbytes>>=1;
	}
}

int drawchar(int x,int y,unsigned char c,unsigned char r) {
	unsigned short fontbyte;
	int bytes;
	signed char * buffer;

	bytes=r==127?1:2;
	if (!r||!c) return 0;
	buffer = blend_add_region(1,x,y,x+8*bytes,y+16);
	if (bytes==1) c--; //TODO off-by-one error in bmp map
	for (int i=15;i>=0;i--) {
		fontbyte=readfont(i,bytes,c,r);
		drawbytes(buffer+8*bytes*(15-i),fontbyte,bytes);
	}
	return bytes*8;
}

static int hiragana_test(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	if (! ((lParam>>30) %2)) {
		if (wParam == VK_F4) {
			msg("in F4");
			drawchar(100,100,38,127);
			return F_REPAINT;
		}
	}
	return F_NONE;
}

static void ro2hi(BYTE * buffer,int * i) {
	//hiramap must be in long->short order for this algorithm
	static FILE * f;
	BYTE c;
	int j,k,n;
	int rb;
	int t[3]={0};
	int seeker;

	if (!f) f=fopen("hiramap.txt","rb");
	if (!f) { msg("Error opening hiramap.txt"); exit(1); }
	//The loop below does this, but with bound check:
	//AND ALSO LITTLE ENDIAN
	//	t[0]=buffer[* i -1]<<24|0x202020;
	//	t[1]=buffer[* i -2]<<24|buffer[* i -1]<<16|0x2020;
	//	t[2]=buffer[* i -3]<<24|buffer[* i -2]<<16|buffer[* i -1]<<8|0x20;
	for (j=0;j<3;j++) {
		for (k=0;k<4;k++) {
			n=* i + (2-j-k);
			rb=0x20;
			if (n>=0 && n<* i) rb=buffer[n];
			t[j]|=rb<<8*(3-k);
		}
	}
	fseek(f,0,SEEK_SET);
	while (fread(& seeker,4,1,f)) {
		for (j=0;j<3;j++) {
			if (seeker==t[j]) {
				* i-=(j+1);
				for (k=0;k<5;k++) { //this needs to hit 0x20
					fread(& c,1,1,f);
					if (c==0x20) return; //sjis lobyte start at 0x40
					buffer[(* i)++]=c;
				}
				msg("should not be here");
				exit(1);
			}
		}
		fseek(f,6,SEEK_CUR); //10 byte alignment
	}
	msg("Not found.");
}

static int hiragana_input(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	static BYTE buffer[100];
	static int i=0;
	static int shifted;
	int x=inp_bx,li,j;
	int r,c;
	int last_backspace;
	int glob=0;
	if (wParam == VK_SHIFT) {
		shifted = ! ((lParam>>31) %2);
		msg("Shifted:%d",shifted);
		return F_NONE;
	}
	if (! ((lParam>>30) %2)) {
		if (wParam == VK_ESCAPE) {
			exit(0);
		} else if (wParam == VK_PRIOR) {
			jmdict_search(-1,0,0,0);
			return F_REPAINT;
		} else if (wParam == VK_NEXT) {
			jmdict_search(1,0,0,0);
			return F_REPAINT;
		} else if (wParam == VK_DELETE) {
			i=0;
		} else if (wParam==VK_RETURN||(wParam=='8'&&shifted)) {
			glob=1;
		} else if (wParam == VK_BACK && i>0) {
			//draw_rectangle(1,inp_bx,inp_by,inp_ex,inp_ey,-1);
			for (j=0;j<i;j+=last_backspace) {
				if (issjis(buffer[j])) last_backspace=2;
				else last_backspace=1;
			}
			i=j-last_backspace;
		} else if (wParam >= 'A' && wParam <= 'Z' && i<94) {
			wParam=tolower(wParam);
			msg("in input with: %c",wParam);
			buffer[i++]=wParam;
			ro2hi(buffer,& i);
		} else {
			return F_NONE;
		}
		draw_rectangle(1,inp_bx-2,inp_by-2,inp_ex+2,inp_ey+2,-1);
		for (li=0;li<i;li++) {
			if (issjis(buffer[li])) {
				sjis2anex(buffer[li],buffer[li+1],& c,& r);
				msg("typed:0x%02x%02x -> %dx%d",buffer[li],buffer[li+1],c,r);
				li++;
			} else {
				sjis2anex(buffer[li],0,& c,& r);
			}
			x+=drawchar(x,inp_by,c,r);
		}
		//draw_rectangle(1,inp_bx,inp_ey,inp_ex,inp_ey+1,-1);
		draw_rectangle(1,x+0,inp_ey,x+2,inp_ey+1,1);
		draw_rectangle(1,x+3,inp_ey,x+5,inp_ey+1,1);
		draw_rectangle(1,x+6,inp_ey,x+8,inp_ey+1,1);
		jmdict_search(0,buffer,i,glob);
		if (glob) {
			sjis2anex('*',0,& c,& r);
			x+=drawchar(x,inp_by,c,r);
		}
		return F_REPAINT;
	}
	return F_NONE;
}

static int hiragana_init(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	char instruction[]="Enter to find all";
	int i,col,row;
	int x=inp_ex-8*strlen(instruction);

	for (i=0;i<2;i++)
		draw_rectangle(i,0,0,game_bitmap_w,game_bitmap_h,-1);
	draw_rectangle(0,inp_bx-2,inp_by-2,inp_ex+2,inp_ey+2,1);
	draw_rectangle(0,inp_bx-1,inp_by-1,inp_ex+1,inp_ey+1,3);
	draw_rectangle(1,inp_bx+0,inp_ey,inp_bx+2,inp_ey+1,1);
	draw_rectangle(1,inp_bx+3,inp_ey,inp_bx+5,inp_ey+1,1);
	draw_rectangle(1,inp_bx+6,inp_ey,inp_bx+8,inp_ey+1,1);
	for (i=0;i<strlen(instruction);i++) {
		sjis2anex(instruction[i],0,& col,& row);
		x+=drawchar(x,inp_ey+2,col,row);
	}
	return F_REPAINT;
}

void hiragana_setup() {
	unsigned int p[10]={
		//AARRGGBB
		0,
		0xffffffff,
		0xffff0000,
		0xff006600,
		0xff0000ff,
		0xffffff00,
		0xff00ffff,
		0xffff00ff,
	};
	blend_palette(10, p);
	add_handler(H_KEY, & hiragana_test);
	add_handler(H_KEY, & hiragana_input);
	add_handler(H_CREATE, & hiragana_init);
}
