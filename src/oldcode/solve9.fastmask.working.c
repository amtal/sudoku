#include "datum.h"
#include "log.h"
#include "dump.h"
#include "timer.h"
#include <stdio.h>

// 9x9
#define GRID_SIZE 81

// This lookup table knocked 1s off a 7.5s run...
static const int blockAddrLookup[81] = {
	 0, 0, 0,	 3, 3, 3,	6, 6, 6,
	 0, 0, 0,	 3, 3, 3,	6, 6, 6,
	 0, 0, 0,	 3, 3, 3,	6, 6, 6,
	27,27,27,	30,30,30,	33,33,33,
	27,27,27,	30,30,30,	33,33,33,
	27,27,27,	30,30,30,	33,33,33,
	54,54,54,	57,57,57,	60,60,60,
	54,54,54,	57,57,57,	60,60,60,
	54,54,54,	57,57,57,	60,60,60
};

static const int blockNumLookup[81] = {
	0,0,0,	1,1,1,	2,2,2,
	0,0,0,	1,1,1,	2,2,2,
	0,0,0,	1,1,1,	2,2,2,
	3,3,3,	4,4,4,	5,5,5,
	3,3,3,	4,4,4,	5,5,5,
	3,3,3,	4,4,4,	5,5,5,
	6,6,6,	7,7,7,	8,8,8,
	6,6,6,	7,7,7,	8,8,8,
	6,6,6,	7,7,7,	8,8,8
};

unsigned int blockMask[9];
unsigned int rowMask[9];
unsigned int colMask[9];

unsigned int grid[GRID_SIZE];
unsigned int pgrid[GRID_SIZE];

// let's use the fact that the + sum and the | sum of the
// cells is equal if there's zero overlap
#define ADD_AND_OR_DIVERGE(a,b,c,d,e,f,g,h,i) ( \
		(grid[a]|grid[b]|grid[c]|grid[d]|grid[e]|grid[f]|grid[g]|grid[h]|grid[i]) != \
		(grid[a]+grid[b]+grid[c]+grid[d]+grid[e]+grid[f]+grid[g]+grid[h]+grid[i]) )
int inline isBlockValid(int j) {
	return (ADD_AND_OR_DIVERGE(j,j+1,j+2, j+9,j+10,j+11, j+18,j+19,j+20));
}
int isValid()
{
// rows
	int i, j;
	for (j=0; j<81; j+=9) {
		if (ADD_AND_OR_DIVERGE(j,j+1,j+2,j+3,j+4,j+5,j+6,j+7,j+8)) {debug("fail row %d\n", j/9); return 0;} 
	}
// columns
	for (j=0; j<9; j++) {
		if (ADD_AND_OR_DIVERGE(j,j+9,j+18,j+27,j+36,j+45,j+54,j+63,j+72)) {debug("fail col %d\n", j); return 0;}
	}
// blocks
	for (i=0; i<9; i+=3) {
	for (j=0; j<81; j+=27) {
		if (isBlockValid(i+j)) {debug("fail block %d:%d\n", i/3,j/9); return 0;}
	}
	}

	return 1;
}


#define OR_SUMMATION(a,b,c,d,e,f,g,h,i) (grid[a]|grid[b]|grid[c]|grid[d]|grid[e]|grid[f]|grid[g]|grid[h]|grid[i])
inline unsigned int summateRow(int i) {int j = i*9; return OR_SUMMATION(j,j+1,j+2,j+3,j+4,j+5,j+6,j+7,j+8);}
inline unsigned int summateCol(int i) {return OR_SUMMATION(i,i+9,i+18,i+27,i+36,i+45,i+54,i+63,i+72);}
inline unsigned int summateBlockRaw(int i) {return OR_SUMMATION(i,i+1,i+2, i+9,i+10,i+11, i+18,i+19,i+20);}
inline unsigned int summateBlockXY(int x,int y) {int i = x*3+y*9; return summateBlockRaw(i);}

/* // Not used!
void fillPGrid()
{
	int i, j;
	// anything is possible
	for (i=0; i<GRID_SIZE; i++) {
		pgrid[i]=0x01FF; // all 9 bits possible
	}

	// subtract row and column possibilities
	for (i=0; i<9; i++) {
		unsigned int sub;
		sub = ~(summateRow(i));
		for (j=0; j<9; j++) {pgrid[i*9+j] &= sub;}
		sub = ~(summateCol(i));
		for (j=0; j<81; j+=9) {pgrid[i+j] &= sub;}
	}

	// subtract block possibilities
	for (i=0; i<3; i++) {for (j=0; j<3; j++) {
		unsigned int sub = ~summateBlockXY(i, j);
		int k, l;
		l = i*3 + j*9;
		for (k=0; k<27; k+=9) {
			pgrid[l+k] &= sub;
			pgrid[l+k+1] &= sub;
			pgrid[l+k+2] &= sub;
		}

	}}

	// add givens that were subtracted in last two steps
	for (i=0; i<GRID_SIZE; i++) {if (grid[i]) {pgrid[i]=grid[i];}}

}
*/

/* // Not used!
void inline logMask(unsigned int t)
{
	int j=0;
	for (j=0; j<9; j++) {logit("%d", (t>>j&1)?1:0);}
}
*/

void initMasks()
{
	// These are possibility masks, 1 means the value is
	// unrestricted on that set.
	int i;
	for (i=0; i<9; i++) {
		rowMask[i] = 0x1ff & ~summateRow(i);
		colMask[i] = 0x1ff & ~summateCol(i);
	}
	
	blockMask[0] = 0x1ff & ~summateBlockRaw(0);
	blockMask[1] = 0x1ff & ~summateBlockRaw(3);
	blockMask[2] = 0x1ff & ~summateBlockRaw(6);
	
	blockMask[3] = 0x1ff & ~summateBlockRaw(27);
	blockMask[4] = 0x1ff & ~summateBlockRaw(30);
	blockMask[5] = 0x1ff & ~summateBlockRaw(33);
	
	blockMask[6] = 0x1ff & ~summateBlockRaw(54);
	blockMask[7] = 0x1ff & ~summateBlockRaw(57);
	blockMask[8] = 0x1ff & ~summateBlockRaw(60);
}

inline unsigned int getFastMask(int i)
{
	return rowMask[i/9] & colMask[i%9] & blockMask[blockNumLookup[i]];
}

void possible(int i, unsigned int val)
{
//	printf("+p: %x at r%d/c%d/b%d\n", val, i/9, i%9, blockNumLookup[i]);
	rowMask[i/9] |= val;
	colMask[i%9] |= val;
	blockMask[blockNumLookup[i]] |= val;
}

void impossible(int i, unsigned int val) 
{
//	printf("-p: %x at r%d/c%d/b%d\n", val, i/9, i%9, blockNumLookup[i]);
	rowMask[i/9] &= ~val;
	colMask[i%9] &= ~val;
	blockMask[blockNumLookup[i]] &= ~val;
}

/* // No longer used in favor of getFastMask
// what the cell CAN be
inline unsigned int getMask(int i)

	unsigned int rowMask = summateRow(i/9);
	//logMask(rowMask); logit(" row %d mask\n", i/9);
	unsigned int colMask = summateCol(i%9);
	//logMask(colMask); logit(" col %d mask\n", i%9);
	//logit("%d %d %d %d %d %d %d %d %d\n", grid[7+0], grid[7+9], grid[7+18], grid[7+27], grid[7+36], grid[7+45], grid[7+54], grid[7+63], grid[7+72]);
	//logMask(summateCol(7)); logit(" test mask\n");
	
	// This is fun... block addresses are 0,3,6, 27,30,33, 54,57,60
	// Can be thought of as x+y
	// 	 x = i/27*27
	//	 y = i%9/3*3
	// What are the odds of a lookup table speeding this thing up?
	//unsigned int blockMask = summateBlockRaw(i/27*27 + i%9/3*3);
	// Turns out lookup table cuts 1s off a 7.5s runtime...
	unsigned int blockMask = summateBlockRaw(blockAddrLookup[i]);

	return 0x1ff & ~(rowMask|colMask|blockMask); // invert impossibilities to get possibilities
	
	//t &= pgrid[i]; // in case there's a predetermined restriction of only one number
	//		//	disregard that these never get masked, or at least shouldn't

}
*/

void bruteSolve()
{
	setTimer(tSOLVE9PREP);
	
	// Rotations/possibly singles elimination?
	
	int i;
	for (i=0; i<GRID_SIZE; i++) {
		// 0=unknown
		// 1=allready known, can only be one value
		//
		// looping over (!pgrid[p]) is safer than over (pgrid[p])
		// since negative values of p will cause seg faults instead of 
		// infinite mystery loops
		if (grid[i]) {pgrid[i]=0;} else {pgrid[i]=1;}
		
		// Some debugging code - start of a singles detector.
		//int sum=0;
		//for (j=0; j<9; j++) {sum += ((pgrid[i]>>j) & 1);}
		//logit("%x\t", pgrid[i]); // possibility log
		//if ((sum==1) && (grid[i]==0)) {logit("#");}// else {logit("%d ", sum);}
		//if (!((i+1)%9)) {logit("\n");}
	}


	// There's three 9-entry arrays that show 9-bit possibility masks for rows,columns,
	// and boxes. Getting a possibility mask for a certain cell is then just a matter of
	// ANDing the three arrays together at relevant positions.
	initMasks();
	

	markTimer(tSOLVE9PREP);
	setTimer(tSOLVE9FINDVAL);

	int p=0; // position pointer in the main solving grid
	while (!pgrid[p]) {p++;} // skip any knowns

	// loop until algorithm reaches last cell
	do {
		// some useful debug code
		//		logit("%d\t\trow", p);
		//		for(i=0;i<9;i++) {logit("%4x", rowMask[i]);} logit("  col");
		//		for(i=0;i<9;i++) {logit("%4x", colMask[i]);} logit("  blk");
		//		for(i=0;i<9;i++) {logit("%4x", blockMask[i]);} logit("\n");

		//		printf("p=%d v=%x mask=%x pos=%d/%d/%d\n", p, v, mask, p/9, p%9, blockNumLookup[p]);
		
		// find the possibility mask for this cell
		unsigned int mask = (getFastMask(p));
		unsigned int v = grid[p];

		// if there are no possibilities, or the current cell value would
		// exceed the possibilities when incremented:
		// 	we've reached a dead end, backtrack
		// otherwise:
		// 	we gotta find a value for the cell
		if ((mask) && ((v<<1)<=mask)) {	
			// find a value for the cell and update it:
			//
			// we'll be updating the cell: this means the current
			// value must be 'uncovered'/'released' since we're changing it
			//
			// Since the algorithm moves forward, and undoes all changes when 
			// backtracking, an update algorithm that only worked on the cells
			// in front would be sweet. I need to look into this further!
			//
			// also, if v is null it's set to 1 so shifting operations work
			if (v) {possible(p, v);} else {v=1;}

			// v&mask is true if v is a valid choice for the cell
			// so, let's find a valid value for v
			//
			// this assumes mask has a valid possibility in it... 
			// it always should though
			while (!(v&mask)) {v = v<<1;}
			// make sure the new value can't be re-used in the same constraints
			impossible(p, v);
			grid[p]=v;
			// skip known cells as usual
			do {p++;} while(!pgrid[p]);
		} else {
			// dead end with no possibilities, let's backtrack
			// 
			// it's not backtracking if you don't undo all the effects
			if (v) {possible(p, v);}
			// since the algorithm counts up, the cell must be zero
			// this also signifies that this cell doesn't affect others in
			// any way shape or form
			grid[p]=0;
			// go down by one, then skip any 'given' cells
			do {p--;} while (!pgrid[p]);
		}
	} while(p<81);
	markTimer(tSOLVE9FINDVAL);

}


void solve9(sudata* data) 
{
	setTimer(tSOLVE9PRE);
	// convert to preferred structure format
	int i;
	for (i=0; i<GRID_SIZE; i++) {
		if (data->grid[i]) {grid[i] = 1 << (data->grid[i]-1);}
		else {grid[i]=0;}
	}
	
	// check validity
	if (isValid()) {logit("Solving valid puzzle.\n"); data->valid=1;} 
	else {logit("Invalid puzzle, ignoring.\n"); data->valid=0;}
	markTimer(tSOLVE9PRE);
	// solve
	bruteSolve();

	setTimer(tSOLVE9POST);
	if (isValid()) {logit("Solution found.\n");}
	else {logit("No solution found.\n");}

	// convert back to generic format
	for (i=0; i<GRID_SIZE; i++) {
		if (!grid[i]) {data->grid[i]=0; continue;} // zero handler
		int j;
		for(j=0; j<9; j++) {
			if ((grid[i]>>j)&1) {
				data->grid[i]=j+1;
				//if (i%9==0) {logit("\n");}
				//logit("%d ", data->grid[i]);
				break;
			}
		}
	//	printf("%d ", data->grid[i]);
	}
	markTimer(tSOLVE9POST);
}


