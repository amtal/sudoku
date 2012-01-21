#ifndef _GLOBAL_H_
#define _GLOBAL_H_

// mostly used to print extra information that's not necessary
// in a speed run - prints warning at startup if defined!
//
// OFF by default
//#define DEBUG 1

	#ifdef DEBUG
// always flush to output: this is sloww and should only be used
// 	to debug segfaults and other such nasties!
//
// OFF by default, turn on to foil some (not all) segfaults
//#define DEBUG_ALWAYSFLUSH 1
//
//
//print pre and post solve sodokus to stdio
//
// OFF by default, gets spammy
//#define DEBUG_PRINTSUDOKU 1
	#endif


// Copy array data to mmapped chunk of shared memory, fork a child to
// split the processing 50/50 with. It's not flexible or fancy but
// since I know the computer this will run on is dual core...
// If I had more time I'd definitely implement something more general.
//
// EXPERIMENTAL: haven't been able to test without others hogging the CPU.
//#define PARALLEL 1

// flip sudokus based on centroid position:
// 	this should foil bruteforce-foiling puzzles with few clues
// 	in the first few rows/cells
// algorithm could be improved though, could flip rows around as well
// to maximize 'early' hints, and do proper rotations as opposed to just
// flipping
//
// ON by default
// TODO: only works for 9x9, need to apply to 16x16
// MAYBE: move transform AFTER reduction by singles algorithms?
// 	pro: better view
// 	con: hard brute force puzzles rarely have easy to find singles,
// overhead from recalculating or transforming possibility arrays
// 	idea: flip twice, first on load and then after reduction but only
// if it's actually worth doing: the overhead when it's not will then be 
// minimal!
#define DO_CENTROID_FLIP 1

// Go through grid and find obvious singles before solving. Makes sense
// on 'human solveable' and bulk puzzles.
//
// ON by default
#define DO_NAKED_SINGLE_REDUCTION 1

// Find rows/columns/blocks where there's only one occurence of a number.
// More computationally expensive, but should pay off in a puzzle with 
// many hints.
//
// ON by default
#define DO_HIDDEN_SINGLE_REDUCTION 1

// Constraint bitboard OR summations are done using a macro if this is
// defined, and for loops otherwise.
//
// Unsure of default. 16x16 only implements for loops right now, leaving
// it on for the 0.1% performance increase in 9x9.
#define UNROLLED_SUMMATIONS 1

// Reads/writes 16x16 Sudokus using the values 0..f for x y positions
// and 1..g for actual values when turned on.
// If off, puzzles are read/written in decimal notation.
//
// OFF by default seems to be project style
//#define FILE_FORMAT_HEX 1


// getconf PAGESIZE on ssh-linux.ece.ubc.ca returns 4096 bytes
// let's use that as the default size for major buffers
#define PAGE_SIZE 4096
// can be overriden by an optional command line argument
#define DEFAULT_OUTFILE "puzzles-sol.txt"


#endif
