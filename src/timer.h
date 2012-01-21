#ifndef _TIMER_H_
#define _TIMER_H_

#define TOTAL_TIMERS 9

#define tLOAD 0
#define tSOLVE 1
#define tSAVE 2

static const char* timerStrings[TOTAL_TIMERS] = {
	"loading and parsing",
	"solving",
	"saving to disk",
};

void inline setTimer(int id);
void inline markTimer(int id);
void printStats();

#endif

