/* Compile: gcc main.c -o ubuntuunicode.exe
 * Run: ubuntuunicode.exe
 */

#define WINVER 0x0500
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

// keys
#define CTRL 0xa2
#define SHIFT 0xa0
#define U 0x55
#define ENTER 0xd
#define SPACE 0x20

// digit range
#define NUMLOW 0x30
#define NUMHIGH 0x39
// char range
#define CHARLOW 0x41
#define CHARHIGH 0x46

#define MAXCHARS 0xFFFFF

// Public variables
bool CTRL_DOWN = false;
bool SHIFT_DOWN = false;
bool U_DOWN = false;
bool ACTIVATED = false;
int SENDCHAR = 0;

void SendKey(int key);
bool isHexChar(int key);
int decodeHexChar(int key);
void PublishAndReset();
void Reset();
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
bool KeyDown(int code);
bool KeyUp(int code);

/**
 * This program implements Ubuntu's unicode input on Windows
 * Read more here:
 * https://en.wikipedia.org/wiki/Unicode_input#In_X11_(Linux_and_Unix_variants) 
 * @return exit
 */
int main(){
 	HINSTANCE instance = GetModuleHandle(NULL);
	// Install the low-level keyboard & mouse hooks
	HHOOK hhkLowLevelKybd = 
		SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, instance, 0);

	// Keep this app running until we're told to stop
	MSG msg;
	while (!GetMessage(&msg, 0, 0, 0)) {	//this while loop keeps the hook
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hhkLowLevelKybd);

	// Exit normally
	return 0;
}

/**
 * Keyboard input callback
 * @param  nCode  A code the hook procedure uses to determine how to process the message
 * @param  wParam The identifier of the keyboard message
 * @param  lParam A pointer to a KBDLLHOOKSTRUCT structure
 * @return        If nCode is less than zero, the hook procedure must return the value
 *                returned by CallNextHookEx
 */
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam){
	bool eatStroke = false;
	
	// Declare a pointer to the KBDLLHOOKSTRUCTdsad
	KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
	int code = pKeyBoard->vkCode;

	if (nCode == HC_ACTION){
		switch (wParam){
			// Key is down
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				eatStroke = KeyDown(code);
				break;
			// Key is up
			case WM_KEYUP:
			case WM_SYSKEYUP:
				eatStroke = KeyUp(code);
				break;
		}
	}

	return eatStroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam);
}

/**
 * Handles key down events
 * @param  code Key pressed
 * @return      Whether or not key should be eaten
 */
bool KeyDown(int code){
	// Change key states
	if(code == CTRL){
		CTRL_DOWN = true;
	} else if(code == SHIFT){
		SHIFT_DOWN = true;
	} 
	// Determine if need to eat key
	else if(CTRL_DOWN && SHIFT_DOWN && code == U){
		// user activated unicode input
		return true;
	} else if(ACTIVATED && isHexChar(code)){
		// user is typing number
		return true;
	} else if(ACTIVATED && !CTRL_DOWN && !SHIFT_DOWN 
			&& !U_DOWN && (code == ENTER || code == SPACE)){
		// The user pressed ctrl+shift+u then typed number, 
		// then pressed enter or space
		return true;
	}

	return false;
}

/**
 * Handles key up events
 * @param  code Key released
 * @return      Whether or not key should be eaten
 */
bool KeyUp(int code){
	if(code == CTRL){
		CTRL_DOWN = false;

		// If user held ctrl+shift while typing number
		// the char should be sent on release
		if(ACTIVATED && SHIFT_DOWN && SENDCHAR > 0){
			PublishAndReset();
		}
	} else if(code == SHIFT){
		SHIFT_DOWN = false;

		// If user held ctrl+shift while typing number
		// the char should be sent on release
		if(ACTIVATED && CTRL_DOWN && SENDCHAR > 0){
			PublishAndReset();
		}
	} 
	// User activated unicode input
	else if(CTRL_DOWN && SHIFT_DOWN && code == U){
		ACTIVATED = true;
		return true;
	} 
	// User typed ctrl+shift+u, then released all keys, then typed number
	// then pressed either enter or space
	else if(ACTIVATED && !CTRL_DOWN && !SHIFT_DOWN 
			&& !U_DOWN && (code == ENTER || code == SPACE)){
		PublishAndReset();
		return true;
	} 
	// User typed ctrl+shift+u and is now typing hex numbers
	else if(ACTIVATED && isHexChar(code)){
		// append input
		SENDCHAR = (SENDCHAR << 4) | decodeHexChar(code);

		// if input is longer than MAXCHARS, just send the char
		if(SENDCHAR > MAXCHARS){
			PublishAndReset();
		}

		return true;
	}
	// User typed something other
	else if(ACTIVATED){
		Reset();
	}

	return false;
}

/**
 * Publishes current char and resets
 */
void PublishAndReset(){
	SendKey(SENDCHAR);
	Reset();
}

/**
 * Resets input
 */
void Reset(){
	ACTIVATED = false;
	SENDCHAR = 0;
}

/**
 * Presses and releases an unicode keystroke
 * @param key Key to send
 */
void SendKey(int key){
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press a unicode "key"
	ip.ki.dwFlags = KEYEVENTF_UNICODE;
	ip.ki.wVk = 0;
	ip.ki.wScan = key;
	SendInput(1, &ip, sizeof(INPUT));
 
	// Release key
	ip.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

/**
 * Decode the hex int from keyboard input
 * @param  key Key to decode
 * @return     Integer of the input
 */
int decodeHexChar(int key){
	if(key <= NUMHIGH){
		return key - (NUMHIGH - 9);
	}
	return key - (CHARHIGH - 5) + 10;
}

/**
 * Determines if a key is a hex code. Characters 0-9 and A-F are hex codes
 * @param  key Key to decode
 * @return     Whether or not key is hex
 */
bool isHexChar(int key){
	return (key >= NUMLOW && key <= NUMHIGH) || (key >= CHARLOW && key <= CHARHIGH);
}