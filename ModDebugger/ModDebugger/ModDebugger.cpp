/*++

OutputDebugStringの文字を取り出す。
32bit/64bitどちらでもコンパイル可能なようにしている。(ソリューションやプロジェクトの設定的にも)
・ターゲットのプロセスの｢クラス名｣がわかっているならば、ビット数の壁を越えられる。
・ターゲットのファイル名しか分かっていないならば、ビット数の壁は超えられない。
　この場合、ターゲットのビット数(32 or 64)と同じビット数でコンパイルしておく必要がある。
なお、OutputDebugString自体の取得は、64bitと32bit混在でも問題なく取ってこれる。

--*/

#include "WinTarget.h"
//---------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>


#include "SetConsole.h"
#include "GetProcess.h"



// 以下ターゲットのクラス名か、プロセスネーム(の一部)のどちらかわかってればなんとかなる。
#define TARGET_CLASS_WND "Tenshouki95"


// メイン関数
int  main( int argc, char ** argv ) {

	SetConsoleAttr();
	
	HANDLE AckEvent;
	HANDLE ReadyEvent;
	HANDLE SharedFile;
	LPVOID SharedMem;
	LPSTR  String;
	DWORD  ret;
	DWORD  LastPid;
	LPDWORD pThisPid;
	BOOL    DidCR;

	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	//--------------- セキュリティ
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = &sd;

	if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
		fprintf(stderr,"unable to InitializeSecurityDescriptor, err == %dn",
			GetLastError());
		exit(1);
	}

	if(!SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE)) {
		fprintf(stderr,"unable to SetSecurityDescriptorDacl, err == %dn",
			GetLastError());
		exit(1);
	}

	// OutputDebugStringで書き込んだよーというイベントを取得するためにイベント作成。
	//--------------- ACKイベント(シグナル状態で、次のOutputDebugString()用意)
	AckEvent = CreateEvent(&sa, FALSE, FALSE, "DBWIN_BUFFER_READY");

	if (!AckEvent) {
		fprintf(stderr,	"ModDebugger: Unable to create synchronization object, err == %dn",
		GetLastError());
		exit(1);
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// fprintf(stderr, "ModDebugger: already runningn");
		// exit(1);
	}

	//--------------- READYイベント(シグナル状態でOutputDebugString()出力が完了)
	ReadyEvent = CreateEvent(&sa, FALSE, FALSE, "DBWIN_DATA_READY");

	if (!ReadyEvent) {
		fprintf(stderr,
			"ModDebugger: Unable to create synchronization object, err == %dn",
			GetLastError());
		exit(1);
	}

	// OutputDebugString()で書き込まれるバッファを共有メモリで開く
	//--------------- "DBWIN_BUFFER"という名称の共有メモリ(4096bytes)
	SharedFile = CreateFileMapping(
		(HANDLE)-1,
		&sa,
		PAGE_READWRITE,
		0,
		4096,
		"DBWIN_BUFFER");

	if (!SharedFile) {
		fprintf(stderr,
			"ModDebugger: Unable to create file mapping object, err == %dn",
			GetLastError());
		exit(1);
	}

	SharedMem = MapViewOfFile(
		SharedFile,
		FILE_MAP_READ,
		0,
		0,
		512);

	if (!SharedMem) {
		fprintf(stderr,
			"ModDebugger: Unable to map shared memory, err == %dn",
			GetLastError());
		exit(1);
	}

	//--------------- 先頭DWORDがプロセスID、以下が格納文字列
	String = (LPSTR)SharedMem + sizeof(DWORD);
	pThisPid = (LPDWORD)SharedMem;

	LastPid = 0xffffffff;
	DidCR = TRUE;

	// 準備完了まで待つ
	SetEvent(AckEvent);

	// 自分のウィンドウを取得するための処理
	HWND hMyWnd;
	{
		char cBuff[1024];
		GetConsoleTitle(cBuff, sizeof(cBuff));
		// 自分のウインドウハンドルを得る
		hMyWnd = FindWindow( "ConsoleWindowClass", cBuff );

	}

	for (;;) {

		// 引数があって、第１番目の引数が"-top"
		if ( argc > 1 && strcmp(argv[1],"-top") == 0 ) {
			// 自分を常に一番手前(Zオーダートップ)に表示
			SetWindowPos(hMyWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		}

		ret = WaitForSingleObject(ReadyEvent, INFINITE);

		if (ret != WAIT_OBJECT_0) {

			fprintf(stderr, "ModDebugger: wait failed; err == %dn", GetLastError());
			exit(1);

		} else {
			// 以下受け取った文字列をどうこうする処理

			// プロセスＩＤが変化した。
			if (LastPid != *pThisPid) {

				LastPid = *pThisPid;
				if (!DidCR) {
					// putchar('n');
					DidCR = TRUE;
				}
			}

			// ターゲットのProcessIDのものだけ表示する。
			// 毎度毎度チェックする必要がある。

			// ターゲットのプロセスＩＤ
			int iTargetProcessID = 0;
			HWND hTargetWnd = FindWindow( TARGET_CLASS_WND, NULL );
			GetWindowThreadProcessId(hTargetWnd, (LPDWORD)&iTargetProcessID);

			// ターゲットプロセスが立ち上がっていない時は、なんでも出す。(邪魔にならないから)
			if ( !iTargetProcessID ) {
				if (DidCR) {
					printf("\n------------------------------------------------ プロセスID:%3u:からの受信\n", LastPid);
				}
				printf("%s", String);

				// 最後の文字が改行ではない場合
				if ( String[strlen(String)-1] != '\n' ) {
					// 改行を出す。
					printf("\n");
				}
				fflush(stdout);

			// ターゲットプロセスが立ち上がっている時には、ターゲットプロセス以外のものは出力しないようにする(邪魔になるから)
			} else if ( iTargetProcessID && iTargetProcessID == *pThisPid ) {
				
				printf("%s", String);

				// 最後の文字が改行ではない場合
				if ( String[strlen(String)-1] != '\n' ) {
					// 改行を出す。
					printf("\n");
				}
				fflush(stdout);
			}

			DidCR = (*String && (String[strlen(String) - 1] == 'n'));

			SetEvent(AckEvent);

		}

	}

	return 0;
}

