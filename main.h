int write_debug=0;
struct temp_block * msgs;
int window_w, window_h;
enum handlers {
	H_BEFORE,
	H_DESTROY,
	H_CREATE,
	H_TIMER,
	H_MOUSE,
	H_KEY,
	H_PAINT_HI,
	H_PAINT_LO,
	H_ENTERSIZEMOVE,
	H_EXITSIZEMOVE,
	H_SIZE,
	H_FLAG,
	H_GATE_CLOSER,
};
enum flags {
	F_NONE,
	F_REPAINT,
};
char * msg(const char * format, ...);
void add_handler(enum handlers type, int (* handler)(HWND,WPARAM,LPARAM));
