# define WIN32_LEAN_AND_MEAN
# include <stdio.h>
# include <stdlib.h>
# include <tchar.h>
# include <ctype.h>
# include <windows.h>
# include "helper.c"
# include "main.h"
# include "blit.h"
# include "blend.h"
# include "blit.c"
# include "blend.c"
# include "draw.c"
# include "layout.c"
# include "jmdict.h"
# include "hiragana.c"
# include "jmdict.c"
# define MAXHANDLERS 10 /*TODO: better solution?*/

char * window_name = "JMdict JA->EN";
int window_w = 320;
int window_h = 480;
static int (* handlers[H_GATE_CLOSER-1][MAXHANDLERS])(HWND,WPARAM,LPARAM);

char * msg(const char * format, ...) {
	static FILE * debug=0; //why do I need =0 here?
	int len;
	va_list args;

/*
	if (!format) {
		len = 0;
		if (!msgs->head) return 0;
		*(msgs->end -1)=0;
		return (char *)mem_finish(0, & len, & msgs);
	}
*/
	if (!write_debug) return 0;
	if (!debug) debug = fopen("debug.txt","w");
	va_start(args, format);
	vfprintf(debug,format,args);
	fprintf(debug,"\r\n");
	fflush(debug);
/*
	len = vsnprintf(0, 0, format, args);
	mem_request(& msgs, len+1);
	vsnprintf(msgs->data, len, format, args);
	*(msgs->end -1)='\n';
*/
	return 0;
}

static void print_msgs() {
	char * m;

	m=msg(0);
	printf("Messages left in buffer:%s\n",m);
	free(m);
}

static int run_handlers(enum handlers type, HWND hwnd, WPARAM wParam, LPARAM lParam) {
	int i;
	enum flags flags;

	flags=0;
	for (i=0;i<MAXHANDLERS && handlers[type][i];i++)
		flags|=handlers[type][i](hwnd,wParam,lParam);
	return flags;
}

void add_handler(enum handlers type, int (* handler)(HWND,WPARAM,LPARAM)) {
	int i;

	for (i=0;i<MAXHANDLERS-1 && handlers[type][i];i++);
	handlers[type][i]=handler;
}

static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	enum handlers call;
	enum flags flags;

	flags=0;
	switch (message) {
		case WM_DESTROY:	call = H_DESTROY;
					PostQuitMessage(0);
					break;
		case WM_CREATE:		call = H_CREATE;
					break;
		case WM_TIMER:		call = H_TIMER;
					break;
		case WM_ERASEBKGND:	//?
					return 1;
		case WM_ACTIVATE:	//?
					return 0;
		case WM_MOUSEMOVE:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:	call = H_MOUSE;
					break;
		case WM_KEYDOWN:
		case WM_KEYUP:		call = H_KEY;
					break;
		case WM_PAINT:		call = H_PAINT_HI;
					break;
		case WM_ENTERSIZEMOVE:	call = H_ENTERSIZEMOVE;
					break;
		case WM_EXITSIZEMOVE:	call = H_EXITSIZEMOVE;
					break;
		case WM_SIZE:		call = H_SIZE;
					break;
		default:		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	msg("Calling %d.",call);
	//flags|=run_handlers(H_BEFORE,hwnd,wParam,lParam);
	flags|=run_handlers(call,hwnd,wParam,lParam);
	if (call==H_PAINT_HI)
		flags|=run_handlers(H_PAINT_LO,hwnd,wParam,lParam);
	if (flags)
		run_handlers(H_FLAG,hwnd,flags,flags);
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
	HWND hwnd;
	MSG messages;
	WNDCLASSEX wincl;
	DWORD dll_file;
	HMODULE mod;
	int i;
	char * token;

	msgs = & empty_block;
	//atexit(print_msgs);
	blit_setup();
	blend_setup(2,-1);
	hiragana_setup();
	jmdict_setup();

	wincl.hInstance = hThisInstance;
	wincl.lpszClassName = window_name;
	wincl.lpfnWndProc = WindowProcedure;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
	if (!RegisterClassEx (& wincl))
		return 0;
	hwnd = CreateWindowEx(
		0,
		window_name, // Classname
		window_name, // Title
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_w, // width
		window_h, // height
		HWND_DESKTOP,
		NULL,
		hThisInstance,
		NULL
	);
	ShowWindow(hwnd, nCmdShow);
	while (GetMessage(& messages, NULL, 0, 0)) {
		TranslateMessage(& messages);
		DispatchMessage(& messages);
	}
	return 0;
}
