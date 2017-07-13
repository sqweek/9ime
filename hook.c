#include <windows.h>
#include <ctype.h>
#include <stdlib.h>

#include "common.h"

typedef struct Node_ {
	char type;
	union {
		long rune;
		struct {
			int n;
			struct Step_ *list;
		} branch;
	} u;
} Node;

typedef struct Step_ {
	char ch;
	struct Node_ next;
} Step;

#define BRANCH(num, lst...) {.type = 'B', .u.branch = {.n = num, .list = (Step[])lst}}
#define STEM(chr, node) {.ch = chr, .next = node}
#define LEAF(r) {.type = 'L', .u.rune = r}

Node root = 
#include "kbd.h"
;

#define MAXHOLD 16

typedef struct HeldChar_ {
	char ch;
	DWORD time;
} HeldChar;

struct {
	char mode;
	union {
		Node *node;
		struct {
			short count;
			char digit[5];
		} hex;
	} u;
	int nheld;
	HeldChar held[MAXHOLD];
} state = {0};

HHOOK nxtHook = 0;

// GetKeyboardState doesn't seem to work, maybe because we never receive focus?
// Managing the shift state manually allows ToAscii to work.
BYTE kbd[256] = {0};

Node*
node_walk(Node *node, char c)
{
	if (node->type == 'B') {
		int i;
		for (i = 0; i < node->u.branch.n; i++) {
			// TODO binary search
			if (node->u.branch.list[i].ch == c) {
				return &node->u.branch.list[i].next;
			}
		}
	}
	return NULL;
}

int
hold(KBDLLHOOKSTRUCT *khook, char ch)
{
	if (state.nheld >= MAXHOLD) {
		return 0;
	}
	state.held[state.nheld++] = (HeldChar){ch, khook->time};
	return 1;
}

char
toChar(KBDLLHOOKSTRUCT *khook)
{
	/* Apparently calling ToAscii on a deadkey in a low-level hook interferes with normal operation... */
	int isdeadkey = (MapVirtualKey(khook->vkCode, MAPVK_VK_TO_CHAR) & 0x80000000);
	if (!isdeadkey) {
		WORD chr;
		if (ToAscii(khook->vkCode, khook->scanCode, kbd, &chr, 0) == 1) {
			return chr;
		}
	}
	return '\0';
}

void
sendrune(long rune)
{
	INPUT in = {.type = INPUT_KEYBOARD};
	in.ki.wScan = rune;
	in.ki.dwFlags = KEYEVENTF_UNICODE;
	SendInput(1, &in, sizeof(INPUT));
}

LRESULT CALLBACK
KeyHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	char chr;
	int isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
	KBDLLHOOKSTRUCT *khook = (KBDLLHOOKSTRUCT*)lParam;
	if (khook->vkCode == VK_LSHIFT || khook->vkCode == VK_RSHIFT) {
		kbd[khook->vkCode] = isDown ? 0x80 : 0x00;
		kbd[VK_SHIFT] = kbd[VK_LSHIFT] | kbd[VK_RSHIFT];
		return CallNextHookEx(nxtHook, nCode, wParam, lParam);
	}
	if (nCode < 0 || !isDown || khook->flags & LLKHF_INJECTED) {
		/* ignore key-up/injected events */
		return CallNextHookEx(nxtHook, nCode, wParam, lParam);
	}
	switch (state.mode) {
	case '\0':
		if (khook->vkCode == VK_RMENU) {
			state.mode = 'T';
			state.u.node = &root;
			state.nheld = 0;
			return -1;
		}
		break;
	case 'T':
		if (chr = toChar(khook)) {
			Node *next = node_walk(state.u.node, chr);
			if (next) {
				if (next->type == 'L') {
					if (next->u.rune == RESERVED_HEX_ESCAPE) {
						state.mode = 'X';
						state.u.hex.count = 0;
						if (hold(khook, chr)) {
							return -1;
						}
					} else {
						sendrune(next->u.rune);
						state.mode = '\0';
						return -1;
					}
				} else if (next->type == 'B') {
					state.u.node = next;
					if (hold(khook, chr)) {
						return -1;
					}
				}
			}
		}
		break;
	case 'X':
		if (chr = toChar(khook)) {
			 if (isxdigit(chr)) {
				state.u.hex.digit[state.u.hex.count++] = chr;
				if (state.u.hex.count == 4) {
					sendrune(strtol(state.u.hex.digit, NULL, 16));
					state.mode = '\0';
					return -1;
				} else if (hold(khook, chr)) {
					return -1;
				}
			}
		}
		break;
	}
	if (state.mode != '\0') {
		/* invalid input; revert to pass-through state */
		state.mode = '\0';
		if (khook->vkCode == VK_ESCAPE) {
			return -1; /* avoid replaying pending keypresses if ESC used to explicitly cancel */
		}
		INPUT in[2*MAXHOLD] = {0};
		int i;
		for (i = 0; i < state.nheld; i++) {
			in[2*i].type = INPUT_KEYBOARD;
			in[2*i].ki.wScan = state.held[i].ch;
			in[2*i].ki.dwFlags = KEYEVENTF_UNICODE;
			in[2*i].ki.time = state.held[i].time;
			/* Need to synthesise a key-up to correctly replay repeated characters */
			in[2*i + 1] = in[2*i];
			in[2*i + 1].ki.dwFlags |= KEYEVENTF_KEYUP;
		}
		SendInput(state.nheld*2, in, sizeof(INPUT));
		state.nheld = 0;
	}
	return CallNextHookEx(nxtHook, nCode, wParam, lParam);
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;

	nxtHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyHook, GetModuleHandle(NULL), 0);
	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
