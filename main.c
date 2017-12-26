#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <stdio.h>
#include <stdlib.h>  
#include <string.h>
#include <windows.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <wchar.h>
#include <Shlwapi.h>
#include "unicode.h"

int install();

int main(int argc, char *argv[]){
	if(argc == 2 && strcmp(argv[1], "--install") == 0){
		install(argv[0]);
	}

	// Free the console so it doesn't show when starting
	FreeConsole();

	// Initialize hooks
	INIT_UNICODE();

	// Run while hook is active
	while(start()){
	}

	// Destroy hooks
	DESTROY_UNICODE();

	return 0;
}

int install(char *currentName){
	size_t newsize = strlen(currentName) + 1; 
	wchar_t wcstring[newsize];
	size_t convertedChars = 0; 
	mbstowcs_s(&convertedChars, wcstring, newsize, currentName, _TRUNCATE);
	wchar_t command[MAX_PATH * 2];
	PWSTR path = NULL;
	SHGetKnownFolderPath(&FOLDERID_Startup, 0, NULL, &path);
	wchar_t cur[MAX_PATH];
    PWSTR shortcutName = L"UbuntufyUnicode";
	GetCurrentDirectoryW(MAX_PATH, cur);
	PathAppendW(cur, wcstring);
	swprintf(command, _TRUNCATE, L"cd %ls && mklink %ls %ls", path, shortcutName, cur);
	_wsystem(command);
	return 0;
}