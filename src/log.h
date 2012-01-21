#ifndef _LOG_H_
#define _LOG_H_

// buffered 'fast' logging system to get around printf's ice-age slowness
//
// logit() will always work, debug() only works if DEBUG defined in debug.h
// be damn careful with format/variables, there's zero typechecking...

// must be called before everything else
void logInit();

void inline logit(const char* format, ...);
void inline debug(const char* format, ...);

// flushes log buffer to stdout in one printf() call
void logFlush();


#endif

