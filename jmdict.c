void sjis2anex(BYTE a,BYTE b,int * col,int * row) {
	if (b) {
		//msg("SJIS %dx%d -> 0x%02x%02x",a,b,a,b);
		if (b>0x7f) b--; //there's a gap in sjis but not in anex bmp
		* col=(a-0x81)*2+1;
		* row=b-0x40;
		if (* row >= 0x5F) {
			//msg("\tSecond row sjis!");
			(* col)++;
			* row-=0x5e;
			//msg("\tSJIS ROW (-0x40 -0x5f +1)=%d", *row);
		} else {
			//msg("\tSJIS ROW (-0x40) =%d", *row);
		}
		* row=0x5e - * row;
		//msg("\tANEXBMP row is %d",*row);
		msg("JPN at col:%d, row:%d\n",* col,* row);
	} else {
		* col=a+1;
		* row=127;
	}
}

int cmpsjis(FILE * f,BYTE * buffer,int buflen,int glob) {
	int j,diff;
	BYTE a,b;

	for (j=0;j<buflen;j++) {
		fread(& a,1,1,f);
		if (a=='|') a=0;
		diff=buffer[j]-a;
		if (diff) return diff;
	}
	if (!glob) {
		fread(& a,1,1,f);
		if (a!='|') diff=-1;
	}
	return diff; //diff is 0
}

void jmdict_search(int page,BYTE * buffer,int i,int glob) {
	static FILE * f;
	static int align=200;
	static char entry[]="Entry ";
	static char entry_of[]=" of ";
	static char entry_val[5];
	static int end;
	static int hits;
	static int cur;
	static int curpage=1;
	BYTE a,b=0;
	int looplimit=20; //log2(.21e6)=12.25
	int lb=0,ub; //lower bound, upper bound
	int diff;
	int x=out_bx,y=out_by;
	int col,row;
	int j;
	int firsthit;
	int found=0;

	if (!f) f=fopen("jm_sc_sort_sjis_pad.txt","rb");
	if (!f) msg("Error opening jmdict\n");
	draw_rectangle(1,out_bx,out_by,out_ex,out_ey,-1);
	if (page==0) {
		curpage=1;
		hits=0;
		if (i<2) return;
		if (!end) {
			fseek(f,0,SEEK_END);
			end=ftell(f);
		}
		ub=end;
		while (--looplimit>0) {
			cur=midmod(lb,ub,align);
			//msg("%d.\tSearch at [%d->%d<-%d]",looplimit,lb,cur,ub);
			{
				fseek(f,cur,SEEK_SET);
				fread(& a,1,1,f);
				fread(& b,1,1,f);
				if (a!='h'||b!=':') {
					msg("bad alignment with dictionary: critical error");
					exit(2);
				}
			}
			diff=cmpsjis(f,buffer,i,glob);
			if (diff<0) ub=cur;
			if (diff>0) lb=cur;
			if (diff) continue;
			found=1;
			//msg("\tOkay!");
			//msg("OK@%d",cur);
			while (!diff) {
				cur-=align;
				fseek(f,cur+2,SEEK_SET);
				diff=cmpsjis(f,buffer,i,glob);
			}
			cur+=align;
			//msg("backwards to %d",ftell(f));
			firsthit=cur;
			diff=0;
			while (!diff) {
				cur+=align;
				hits++;
				fseek(f,cur+2,SEEK_SET);
				diff=cmpsjis(f,buffer,i,glob);
			}
			//msg("forwards to %d",ftell(f));
			cur=firsthit;
			fseek(f,cur,SEEK_SET);
			break;
		}
		//msg("Not found in dictionary.");
		if (!found) return;
	} else {
		curpage+=page;
		cur+=align*page;
		fseek(f,cur,SEEK_SET);
	}
	//msg("Displaying at ftell %d",ftell(f));
	//msg("%d hits",hits);
	for (j=0;j<strlen(entry);j++) {
		sjis2anex(entry[j],0,& col,& row);
		x+=drawchar(x,y,col,row);
	}
	snprintf(entry_val,5,"%d",curpage);
	for (j=0;j<strlen(entry_val);j++) {
		sjis2anex(entry_val[j],0,& col,& row);
		x+=drawchar(x,y,col,row);
	}
	for (j=0;j<strlen(entry_of);j++) {
		sjis2anex(entry_of[j],0,& col,& row);
		x+=drawchar(x,y,col,row);
	}
	snprintf(entry_val,5,"%d",hits);
	for (j=0;j<strlen(entry_val);j++) {
		sjis2anex(entry_val[j],0,& col,& row);
		x+=drawchar(x,y,col,row);
	}
	for (j=out_bx-1;j<out_ex;j+=2) {
		draw_rectangle(1,j,y+17,j+3,y+18,1);
		j+=3;
	}
	y+=18; x=out_bx;
	for (j=0;j<199;j++) {
		if (y>game_bitmap_h-18) return;
		b=0;
		fread(& a,1,1,f);
		if (a=='|') {
			y+=18; x=out_bx+16;
			continue;
		} else if (issjis(a)) {
			fread(& b,1,1,f);
			j++;
		}
		sjis2anex(a,b,& col,& row);
		x+=drawchar(x,y,col,row);
		if (x+16>out_ex) { y+=18; x=out_bx; }
	}
}

static void jmdict_get1b(int * col,int * row) {
	static FILE * f;
	BYTE a,b=0;

	if (!f) f=fopen("jm_sc_sort_sjis_pad.txt","rb");
	if (!f) msg("Error opening jmdict\n");
	fread(& a,1,1,f);
	if (issjis(a)) {
		fread(& b,1,1,f);
	}
	sjis2anex(a,b,col,row);
}

static int jmdict_test(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	static int x=1;
	int col,row;

	if (! ((lParam>>30) %2)) {
		if (wParam == VK_F5) {
			msg("in F5");
			jmdict_get1b(& col,& row);
			x+=drawchar(x,150,col,row);
			return F_REPAINT;
		}
	}
	return F_NONE;
}

static int jmdict_init(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	draw_rectangle(0,out_bx-2,out_by-2,out_ex+2,out_ey+2,5);
	draw_rectangle(0,out_bx-1,out_by-1,out_ex+1,out_ey+1,4);
	return F_REPAINT;
}

void jmdict_setup() {
	add_handler(H_KEY, & jmdict_test);
	add_handler(H_CREATE, & jmdict_init);
}
