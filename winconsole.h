#ifndef _WINCONSOLE_H
#define _WINCONSOLE_H

#include <stdio.h>
#include <io.h> // Console Generation(_open_osfhandle(), etc)
#include <fcntl.h> // _O_TEXT

/////////////////
// CONSOLE WINDOW

	///////////////////////////////////////////////////////////////////////////
	// Creates and displays a console window.
	// You can then call writeCon() to display to the console.
	HANDLE initCon(void) {
		// Create console window.
		AllocConsole();
		// Return handle to it.
		return GetStdHandle(STD_OUTPUT_HANDLE);
	}
	///////////////////////////////////////////////////////////////////////////
	// Call mapCon() immediately after initCon()
	// to map input and output to our new console window.
	// You need this so printf()'s and scanf()'s and such
	// will work properly. Otherwise, use writeCon() and readCon().
	void mapCon(HANDLE conHandle) {

		// Output. printf()
		int hCrt = _open_osfhandle( (long)conHandle, _O_TEXT );
		FILE *hf_out = _fdopen(hCrt, "w");
		setvbuf(hf_out, NULL, _IONBF, 1);
		*stdout = *hf_out;

		// Input. scanf()
		HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
		hCrt = _open_osfhandle( (long)handle_in, _O_TEXT );
		FILE *hf_in = _fdopen(hCrt, "r");
		setvbuf(hf_in, NULL, _IONBF, 128);
		*stdin = *hf_in;
	}
	///////////////////////////////////////////////////////////////////////////
	// Writes a string to the console window.
	void writeCon(HANDLE conHandle, const char *str) {
		DWORD charsWritten;
		WriteConsole(conHandle, str, strlen(str), &charsWritten, NULL);
	}
	///////////////////////////////////////////////////////////////////////////
	// Reads a command from the console window.
	void readCon(HANDLE conHandle, char *buff, int numCharsToRead, DWORD &charsRead) {
		ReadConsole(conHandle, buff, numCharsToRead, &charsRead, NULL);
	}
	///////////////////////////////////////////////////////////////////////////
	// Call before application exit to free this console window.
	void cleanCon(void) {
		FreeConsole();
	}


// CONSOLE WINDOW
/////////////////

//////////
// Thread

// This code came about because calls to printf()
// in a tight thread loop serverely slowed the
// calling thread down. So we create a string
// printing thread to load the console and
// print to it. You'll still get some slow-down
// but it a least won't take up ALL of the
// calling thread's time.
//
// Now for the next issue. Back2Back calls to
// printStr() will cause prints to be missed.
// The memcpy() and printf() under the hood take
// a while to finish, so it might skip printStr()
// number 2.
//
// TODO: Add mechanism to buffer strings passed
// to printStr().

// Usage: Call TrySubmitThreadpoolCallback(). Then
// you can call printStr() to send strings to print
// to the thread. Set threadRun to false to shutdown
// the thread entirely. You'll have to set threadRun
// to true then call TrySubmit() again to restart it.

/*
	// Prints strings that are in ptStrBuffer.
	static bool threadRun = true;
	const int PTPRINTING = 0;
	const int PTCOPYING = 1;
	const int PTIDLE = 2;
	const int PTSTRREADY = 3;
	static int ptState = PTIDLE;
	char ptStrBuffer[100] = {0};
	void CALLBACK printerThread(PTP_CALLBACK_INSTANCE instance, void *context) {

		// Console window. Init and map printf/scanf().
		HANDLE consoleHandle = initCon();
		mapCon(consoleHandle);

		while(threadRun) {
			if( ptState == PTSTRREADY ) {
				ptState = PTPRINTING;
					printf(ptStrBuffer);
					memset( ptStrBuffer, 0, sizeof(ptStrBuffer) );
				ptState = PTIDLE;
			}
		}

		// Clean up console window.
		cleanCon();
	}
	// Helper func for printerThread.
	void printStr(const char *_str) {
		if( ptState == PTIDLE ) {
			ptState = PTCOPYING;
				memcpy(ptStrBuffer, _str, strlen(_str));
			ptState = PTSTRREADY;
		}
	}


	// Create string printing thread.
	TrySubmitThreadpoolCallback(printerThread, 0, NULL);
*/
// Thread
//////////

#endif // _WINCONSOLE_H
