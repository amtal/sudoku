#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"
#include "datum.h"
#include "diskrw.h"
#include "solve9.h"
#include "solve16.h"
#include "timer.h"
#include "global.h"
#ifdef PARALLEL
#include <string.h> // this is where memcpy lives??
#include <wait.h>
#endif

void exitFunc() 
{	// ensures everything always gets printed, regardless
	// of exit method (hey new euphemism - 'segfaults' are
	// really 'exit methods'!)
	logFlush(); 
}

int main(int argc, char* argv[])
{	//
	// General logic:
	// 
	// load puzzles from input .txt into NxN byte arrays
	// for every puzzle:
	// 	pass to size-specific solver
	// 		convert to solver-specific data structure
	// 		validate
	// 		solve
	// dump puzzles to drive
	//
	// Optimization notes: 
	//
	// System calls are slow, use memory buffers and read/dump them in one go.
	// If necessary, have a 'fast' and 'slow' method to do something. Call
	// 	slow method if fast one fails to find the reason.
	// Avoid multiplication and division via lookup tables?
	// Localize calculations, abstract and isolate them. The smaller the code is,
	// 	the easier it should be to improve...
	// Bit masks and binary logic instead of integer operations.
	// Inline everything possible.
	// const/static/register keywords?
	//

	setTimer(tLOAD);

	logInit();
	#ifdef DO_HIDDEN_SINGLE_REDUCTION
	initHiddenLookup(); // initialize lookup tables
	initHiddenLookup16();
	#endif
	if (atexit(&exitFunc)) {logit("Caution: atexit() exit handler returned non-zero. Output might not be flushed on exit.\n");}

	// checks args for file string, error if DNE
	char* input;
	char* outfile;
	outfile = DEFAULT_OUTFILE;
	switch (argc) {
		case 3:
			outfile = argv[2];
		case 2:
			input = argv[1];
		break;
		default:
			logit("Usage: sudoku-solve INPUT [OUTPUT]\n\
Solves 9x9 or 16x16 sudoku puzzles in a minimum amount of time.\n\
\n\
Output defaults to %s, solver configuration is hardcoded. (See global.h)\n", DEFAULT_OUTFILE);
			exit(1);

	}

	logit("Sudoku solver by Alexander Kropivny. All rights reserved.\n");

	// pass file name, sudoku buffers to load function
	//sudata head; head.next = 0;
	sudata* head = load(input);

	markTimer(tLOAD);
	
	setTimer(tSOLVE);
	logit(". for successful solve, ! for invalid puzzle, # for solver failure:\n");
	#ifdef PARALLEL
	sudata buffer;
	// fork
	pid_t pid;
	pid = fork();
	if (pid<0) {logit("Error, fork failed!\n"); exit(1);}
	int even9,even16;
	if (pid==0) {even9=even16=1; /*printf("child\n");*/} else {even9=even16=0; /*printf("parent\n");*/}
	#endif	
	// pass sudoku buffers to solver
	sudata* p;
	for (p=head; (int)p!=0; p=p->next) {
		if (p->width==9) {
			#ifdef PARALLEL
			if (even9) {even9=0; continue;} else {even9=1;}
			memcpy(&buffer, p, sizeof(sudata));
			solve9(&buffer);
			memcpy(p, &buffer, sizeof(sudata));
			#else
			solve9(p);
			#endif
		} else {
			#ifdef PARALLEL
			if (even16) {even16=0; continue;} else {even16=1;}
			memcpy(&buffer, p, sizeof(sudata));
			solve16(&buffer);
			memcpy(p, &buffer, sizeof(sudata));
			#else
			solve16(p);
			#endif
		}
		//if (p->width==9) {solve9(p);}
	}
	#ifdef PARALLEL
	// wait for child
	if (pid==0) {_exit(0);} else {
		int cstatus;
		//logit("parent waiting for child\n");
		wait(&cstatus);
		//logit("parent done waiting child returned %d\n", cstatus);
	}
	#endif
	logit("\n");
	markTimer(tSOLVE);
	setTimer(tSAVE);

	//dump solved sudokus
	save(outfile, head);

	markTimer(tSAVE);

	printStats();
	logit("Run complete. Exiting.\n");
	logFlush();
	return 0;
}







