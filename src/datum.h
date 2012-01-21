#ifndef _DATUM_H_
#define _DATUM_H_

#define SIZE_4x 256
#define SIZE_3x 81

// List of puzzles. Easy to pass around, easy to operate on...
typedef struct {
	char grid[SIZE_4x]; // 2d array of chars, either 16x16 or 9x9
	int width;  // if this isn't 9 it's assumed to be 16

	void* next; // null terminates list
	int valid;  // 0 for invalid, set by solver
	
	// 2x2 rotation matrix used to foil brute force-foiling
	int rotmat[4]; // puzzles with few hints/givens at the start
	// I ought to move this from loading to solving: the x y val
	// format is ideal for performing rotations, but I ended up
	// rotating after it was put into NxN grid form....
} sudata;


#endif

