#include "WinTarget.h"

#include <stdio.h>

/*
FOREGROUND_BLACK   = 0x00;
FOREGROUND_BLUE	   = 0x01;

FOREGROUND_GREEN   = 0x02;
FOREGROUND_RED     = 0x04;
FOREGROUND_CYAN    = FOREGROUND_BLUE  | FOREGROUND_GREEN;
FOREGROUND_MAGENTA = FOREGROUND_BLUE  | FOREGROUND_RED;
FOREGROUND_YELLOW  = FOREGROUND_GREEN | FOREGROUND_RED;
FOREGROUND_WHITE   = FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_RED;
FOREGROUND_INTENSITY = 0x08;

BACKGROUND_BLACK   = 0x00;
BACKGROUND_BLUE    = 0x10;
BACKGROUND_GREEN   = 0x20;
BACKGROUND_RED     = 0x40;
BACKGROUND_CYAN    = BACKGROUND_BLUE  | BACKGROUND_GREEN;
BACKGROUND_MAGENTA = BACKGROUND_BLUE  | BACKGROUND_RED;
BACKGROUND_YELLOW  = BACKGROUND_GREEN | BACKGROUND_RED;
BACKGROUND_WHITE   = BACKGROUND_BLUE  | BACKGROUND_GREEN | BACKGROUND_RED;
BACKGROUND_INTENSITY = 0x80;
*/




// コンソール画面の設定など。XP以上
void SetConsoleAttr() {

	HINSTANCE hLib = LoadLibrary("KERNEL32.DLL");

	BOOL (WINAPI* GetConsoleFontInfo)(HANDLE,BOOL,DWORD,PCONSOLE_FONT_INFO) = 
		(BOOL (WINAPI*)(HANDLE,BOOL,DWORD,PCONSOLE_FONT_INFO))GetProcAddress(hLib, "GetConsoleFontInfo");

	DWORD  (WINAPI* GetNumberOfConsoleFonts)(VOID) = 
		(DWORD (WINAPI*)(VOID))GetProcAddress(hLib, "GetNumberOfConsoleFonts");

	BOOL (WINAPI* SetConsoleFont)(HANDLE, DWORD) = 
		(BOOL(WINAPI*)(HANDLE, DWORD))GetProcAddress(hLib, "SetConsoleFont");


	//-------------------------------------------------------------------------
	HANDLE hConsoleScreen = GetStdHandle(STD_OUTPUT_HANDLE);

	// コンソールのプロパティのフォントの項目で設定できる、縦横サイズ
	int fontHeight = 13;
	int fontWidth = 6;

	DWORD fontNum = GetNumberOfConsoleFonts();
	CONSOLE_FONT_INFO* fonts = (CONSOLE_FONT_INFO*)malloc(sizeof(CONSOLE_FONT_INFO) * fontNum);

	GetConsoleFontInfo(hConsoleScreen, FALSE, fontNum, fonts);

	for(int index = 0; index < (int)fontNum; ++index)
	{
		fonts[index].dwFontSize = GetConsoleFontSize(hConsoleScreen, fonts[index].nFont);
		// printf("%d, %d\n", fonts[index].dwFontSize.Y, fonts[index].dwFontSize.X);
		if(fonts[index].dwFontSize.Y == fontHeight && fonts[index].dwFontSize.X == fontWidth)
		{
			SetConsoleFont(hConsoleScreen, fonts[index].nFont);
			break;
		}
	}

	free(fonts);

	//-------------------------------------------------------------------------

	// サイズや位置(フォントの影響を受けるので思うようにいかない…)
	SMALL_RECT rctWindowRect = {0,0,100,30};
	COORD dwCoord;  // コンソールのバッファサイズ用
	dwCoord.X = 101;  
	dwCoord.Y = 2000;

	SetConsoleScreenBufferSize(hConsoleScreen, dwCoord);
	SetConsoleWindowInfo(hConsoleScreen,TRUE,&rctWindowRect);


	//-------------------------------------------------------------------------
	FreeLibrary(hLib);
}



/*
// コンソール画面の設定など。Vista以上用
void SetConsoleAttr() {

	// if ( getOSVersion() < 6 ) { // Vista未満なら何もしない
		SetConsoleAttrForWinXP();
		return;
	// }
	//-------------------------------------------------------------------------


	// フォントの大きさ
	BOOL bRtn;
	HANDLE hConsoleScreen = GetStdHandle(STD_OUTPUT_HANDLE);

	int nFontIndex = 3; // size '12'

	// http://msdn.microsoft.com/en-us/library/ms686036(v=vs.85).aspx
	// "if the current font is a raster font, SetConsoleOutputCP does
	//  not affect how extended characters are displayed."
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = nFontIndex;
	cfi.dwFontSize = GetConsoleFontSize(hConsoleScreen, nFontIndex);
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = 500;
	wcscpy_s(cfi.FaceName, ARRAYSIZE(cfi.FaceName), L"MSGothic");

	SetCurrentConsoleFontEx(hConsoleScreen, FALSE, &cfi);


	// サイズや位置(フォントの影響を受けるので思うようにいかない…)
	SMALL_RECT rctWindowRect = {0,0,100,30};
	COORD dwCoord;  // コンソールのバッファサイズ用
	dwCoord.X = 101;  
	dwCoord.Y = 2000;

	bRtn = SetConsoleScreenBufferSize(hConsoleScreen, dwCoord);
	bRtn = SetConsoleWindowInfo(hConsoleScreen,TRUE,&rctWindowRect);


}
*/