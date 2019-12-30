#include <stdio.h>
#include <stdarg.h>

void log2(const char *msg) {
	if (msg) {
		FILE *fp = fopen("debug2.log", "w+");
		fprintf(fp, msg);
		fprintf(fp, "\n");
		fclose(fp);
	}
}

void logf(const char *lpszFormat, ...) {
	try {
		static char szMsg[2048];
		va_list argList;
		va_start(argList, lpszFormat);
		vsprintf(szMsg, lpszFormat, argList);
		va_end(argList);
		log2(szMsg);
	} catch (...) {}
}
