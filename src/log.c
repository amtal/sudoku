#include <stdio.h> // printf
#include <stdarg.h> // va_args
#include <time.h> // time structs/gettimeofday
#include <sys/time.h>
#include "global.h"

// logit() more than this much text at a time, and there's
// a risk of a buffer overrun
#define OVERRUN_MARGIN 80

char buffer[PAGE_SIZE];

char* pen; // pointer to current write location
char* end;

// flushes log buffer to stdout in one printf() call
void logFlush()
{
	if (pen==buffer) return;
	printf("%s", buffer);
	pen = buffer;
}

void inline logit(const char* format, ...)
{
	if (pen>=end) { // Buffer too full, flush it.
		logFlush();
	}

	va_list args;

	va_start(args, format);
	int written = vsprintf(pen, format, args);
	va_end(args);

	pen += written;

	#ifdef DEBUG_ALWAYSFLUSH
	logFlush();
	#endif
}

void inline debug(const char* format, ...)
{
#ifdef DEBUG
	if (pen>=end) {logFlush();}

	va_list args;

	// Is there a better way to do this? WHO CARES, debug function!
	va_start(args, format);
	int written = vsprintf(pen, format, args);
	va_end(args);

	pen += written;

	#ifdef DEBUG_ALWAYSFLUSH
	logFlush();
	#endif
#endif
}

// must be called before everything else
void logInit()
{
	buffer[0] = 0; // 0-length string
	pen = buffer;
	end = buffer+PAGE_SIZE-OVERRUN_MARGIN;
	#ifdef DEBUG
	logit("!!WARNING!!  DEBUG is defined in global.h!  !!WARNING!!\n");
	#endif
	#ifdef DEBUG_ALWAYSFLUSH
	logit("!!CAUTION!! DEBUG_ALWAYSFLUSH also defined! !!CAUTION!!\n");
	#endif
}



