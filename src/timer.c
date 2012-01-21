#include <time.h> // time structs/gettimeofday
#include <sys/time.h>
#include "global.h"
#include "log.h"
#include "timer.h"

struct timeval running[TOTAL_TIMERS];
struct timeval total[TOTAL_TIMERS];
struct timeval now;

void inline setTimer(int id) 
{
	gettimeofday(&running[id], 0);
}

void inline markTimer(int id) 
{
	gettimeofday(&now, 0);
	total[id].tv_sec = now.tv_sec - running[id].tv_sec;
	if (now.tv_usec<running[id].tv_usec) {
		total[id].tv_sec--;
		total[id].tv_usec = 1000000+now.tv_usec-running[id].tv_usec;
	} else {
		total[id].tv_usec = now.tv_usec - running[id].tv_usec;
	}

	if (total[id].tv_usec>1000000) {
		total[id].tv_sec++;
		total[id].tv_sec -= 1000000;
	}
}

void printStats()
{
	struct timeval sum;
	sum.tv_sec=0;
	sum.tv_usec=0;

	int i;
	for (i=0; i<TOTAL_TIMERS; i++) {
		if (timerStrings[i]) {
		// if timer string==0 there's no timer here
			logit("%03d.%03ds %s\n", total[i].tv_sec, total[i].tv_usec/1000, timerStrings[i]);
			sum.tv_sec += total[i].tv_sec;
			sum.tv_usec += total[i].tv_usec;
			if (sum.tv_usec>1000000) {sum.tv_usec-=1000000; sum.tv_sec++;}
		}
	}

	logit(" --\n%03d.%03ds total\n", sum.tv_sec, sum.tv_usec/1000);
}
