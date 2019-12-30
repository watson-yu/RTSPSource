// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <conio.h>
#include "main.h"

#define LOG(x, ...) log(__FUNCTION__, x, ##__VA_ARGS__) 

void log(const char *functionName, const char *lpszFormat, ...) {
	static char szMsg[512];
	va_list argList;
	va_start(argList, lpszFormat);
	try {
		vsprintf_s(szMsg,lpszFormat, argList);
	} catch(...) {
		strcpy_s(szMsg ,"DebugHelper:Invalid string format!");
	}
	va_end(argList);
	std::string logMsg = static_cast<std::ostringstream*>( &(std::ostringstream() << ::GetCurrentProcessId() << "," << ::GetCurrentThreadId() << "," << functionName << ", " <<  szMsg) )->str();
	std::cout << logMsg << '\n';
}

int _tmain(int argc, _TCHAR* argv[])
{
	int rc = -1;
	const char* url = "rtsp://192.168.1.1/h264?w=320&h=240&fps=15&br=200000";

	CstreamMedia *streamMedia = new CstreamMedia();
	LOG("streamMedia=%d", streamMedia);

	rc = streamMedia->rtspClientOpenStream(url);
	LOG("open stream result=%d", rc);

	rc = streamMedia->rtspClientPlayStream(url);
	LOG("play stream result=%d", rc);

	BYTE data[655360];
	long lDataLen = 655360;
	while (TRUE) {
		rc = streamMedia->GetFrame(data, lDataLen);
		LOG("get frame result=%d", rc);

		//std::cout << '\n' << "Press a key to continue...";
		//if (std::cin.get() == 'c') break;
	}

	return 0;
}

