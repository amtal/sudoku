#include "datum.h"
#include "log.h"
#include "dump.h"
#include <stdio.h>
#include "single.h"
#include "global.h"
#include "lookup16.h"

#define BLOCK 4
#define WIDTH 16
#define GRID_SIZE 256
#define MAXMASK 0xffff

// interestingly, using unsigned short seems marginally SLOWER
// than unsigned int. More efficient cpu instructions? Optimization?
typedef unsigned int bitboard;


// There are WIDTH boxes on a sodku grid. This gives the
// top left corner cell of box n inside a GRID_SIZE array.
static const unsigned char blockShortAddrLookup[WIDTH] = {
	0,4,8,12,	64,68,72,76,  128,132,136,140,  192,196,200,204
};

// Each box has WIDTH elements. This gives the offset of
// box cell n inside a GRID_SIZE array RELATIVE TO the
// top left corner.
static const unsigned char blockShortOffsetLookup[WIDTH] = {
	0,1,2,3,  16,17,18,19,	32,33,34,35,	48,49,50,51
};


// 9 rows/columns/blocks, mask for each
// cell constraint = row+block+col constraints
// updated with possibleYY(), impossibleYY()
// initialized with initMasksYY() (or on initial grid load, same speed though)
bitboard blockMask[WIDTH];
bitboard rowMask[WIDTH];
bitboard colMask[WIDTH];

// our 'scratch space' where we search for a solution
bitboard grid[GRID_SIZE];
// possibility grid, where 1 bit means there's a chance of an entry
//bitboard pgrid[GRID_SIZE];	// currently not used
// restriction grid, where 1 means there's no restriction, 0 means there is
// (set by initial conditions)
// loop over this by using ~ (random memory is less likely to ==0, this results
// in segfaults rather than infinite loops)
unsigned char modifiable[GRID_SIZE];

// let's use the fact that the + sum and the | sum of the
// cells is equal if there's zero overlap
//
// note that 0 cells do not get detected as an error this way!
//
#define ADD_AND_OR_DIVERGE(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p) ( \
		(grid[a]|grid[b]|grid[c]|grid[d]|  grid[e]|grid[f]|grid[g]|grid[h]|  grid[i]|grid[j]|grid[k]|grid[l]|  grid[m]|grid[n]|grid[o]|grid[p]) != \
		(grid[a]+grid[b]+grid[c]+grid[d]+  grid[e]+grid[f]+grid[g]+grid[h]+  grid[i]+grid[j]+grid[k]+grid[l]+  grid[m]+grid[n]+grid[o]+grid[p]) )
int  isBlockValidYY(int j) {
	return (ADD_AND_OR_DIVERGE(j,j+1,j+2,j+3, j+16,j+17,j+18,j+19, j+32,j+33,j+34,j+35, j+48,j+49,j+50,j+51));
}
int  isValidYY(int testCompletion)
{
// rows
	int i, j;
	for (j=0; j<GRID_SIZE; j+=WIDTH) {
		if (ADD_AND_OR_DIVERGE(j,j+1,j+2,j+3,j+4,j+5,j+6,j+7,j+8,j+9,j+10,j+11,j+12,j+13,j+14,j+15)) {debug("fail row %d\n", j/WIDTH); return 0;} 
	}
// columns
	for (j=0; j<WIDTH; j++) {
		if (ADD_AND_OR_DIVERGE(j,j+16,j+32,j+48,  j+64,j+80,j+96,j+112,  j+128,j+144,j+160,j+176,  j+192,j+208,j+224,j+240)) {debug("fail col %d\n", j); return 0;}
	}
// blocks
	for (i=0; i<WIDTH; i+=BLOCK) {
	for (j=0; j<GRID_SIZE; j+=(BLOCK*WIDTH)) {
		if (isBlockValidYY(i+j)) {debug("fail block %d:%d\n", i/BLOCK,j/WIDTH); return 0;}
	}
	}
// blank cells
	if (testCompletion) for (i=0; i<GRID_SIZE; i++) {
		if (!grid[i]) {debug("fail cell %d\n", i); return 0;}
	}	
	return 1;
}


inline bitboard summateRowYY(int r) {
	register bitboard* offset = grid+r*WIDTH; register bitboard sum = 0; register int i;
	for (i=0; i<WIDTH; i++) {sum |= offset[i];}
	return sum;
}
inline bitboard summateColYY(int c) {
	register bitboard* offset = grid+c; register bitboard sum = 0; register int i;
	for (i=0; i<GRID_SIZE; i+=WIDTH) {sum |= offset[i];}	
	return sum;
}
inline bitboard summateBlockRawYY(int b) {
	register bitboard* offset = grid+b; register bitboard sum = 0; register int i,j;
	for (i=0;i<BLOCK;i++) for (j=0; j<BLOCK*WIDTH; j+=WIDTH) {sum |= offset[i+j];}
	return sum; //	return OR_SUMMATION(i,i+1,i+2, i+9,i+10,i+11, i+18,i+19,i+20);

}
inline bitboard summateBlockXYYY(int x,int y) {int i = x*BLOCK+y*WIDTH; return summateBlockRawYY(i);}

/* // Not used!
void inline logMask(bitboard t)
{
	int j=0;
	for (j=0; j<WIDTH; j++) {logit("%d", (t>>j&1)?1:0);}
}
*/

void inline initMasksYY()
{
	// These are possibility masks, 1 means the value is
	// unrestricted on that set.
	int i;
	for (i=0; i<WIDTH; i++) {
		rowMask[i] = MAXMASK & ~summateRowYY(i);
		colMask[i] = MAXMASK & ~summateColYY(i);
		blockMask[i] = MAXMASK & ~summateBlockRawYY(blockShortAddrLookup[i]);
	}
	
}

inline bitboard getFastMaskYY(int i)
{
	return rowMask[i/WIDTH] & colMask[i%WIDTH] & blockMask[blockNumLookup[i]];
}

void inline possibleYY(int i, bitboard val)
{
//	printf("+p: %x at r%d/c%d/b%d\n", val, i/WIDTH, i%WIDTH, blockNumLookup[i]);
	rowMask[i/WIDTH] |= val;
	colMask[i%WIDTH] |= val;
	blockMask[blockNumLookup[i]] |= val;
}

void inline impossibleYY(int i, bitboard val) 
{
//	printf("-p: %x at r%d/c%d/b%d\n", val, i/WIDTH, i%WIDTH, blockNumLookup[i]);
	rowMask[i/WIDTH] &= ~val;
	colMask[i%WIDTH] &= ~val;
	blockMask[blockNumLookup[i]] &= ~val;
}

#ifdef DO_HIDDEN_SINGLE_REDUCTION
// This array looks up grid position based on
// column[a], or row[b], or row[c]. These three 
// arrays are grouped together into one structure to
// make it easy to loop over them.
//
// [3] is constraint type: r, c, and blocks
//   [WIDTH] is different constraints of the same type
//      [WIDTH] is cells within a single constraint
static unsigned char hiddenLookup[3][WIDTH][WIDTH];
#define HS_ROW 0
#define HS_COL 1
#define HS_BLK 2
// This function need only be called once during the program's
// existance.
void initHiddenLookup16()
{
	int i,j;
	for (i=0;i<WIDTH;i++) { // iterate over all constraints
		for (j=0;j<WIDTH;j++) { // iterate within constraint
			hiddenLookup[HS_ROW][i][j] = i*WIDTH+j;
			hiddenLookup[HS_COL][i][j] = i+j*WIDTH;
			// A lookup table generated based on lookup tables?
			// No, I haven't gone crazy. This is a remnant of the
			// original system that ran three separate but extremely
			// similar HS algorithms one after another.
			//
			// This system will run just one algorithm three times,
			// using these lookup tables to cover all three constraint
			// types.
			hiddenLookup[HS_BLK][i][j] = 
				blockShortAddrLookup[i] + blockShortOffsetLookup[j];
		}
	}
}
inline int detectHiddenSinglesYY()
{
	// Hidden single detection algorithm.
	// Loop through constrained set, gathering bits into
	// accumulator. Bits in both accumulator and current
	// cell are added to the repeater. Naked singles are
	// then just ~repeater.
	int type, i; int j; int found=0;

	// loop over rows, columns, block constraint types
	for (type=0; type<3; type++) { 
		// loop over the WIDTH individual constraints
		for (i=0; i<WIDTH; i++) {
			bitboard accumulator=0, repeater=0;
			// loop through individual cells inside a constraint set
			for (j=0; j<WIDTH; j++) {
				int pos = hiddenLookup[type][i][j];
				bitboard cell = getFastMaskYY(pos); //pgrid[pos];
				repeater |= (accumulator & cell) | grid[pos];
				accumulator |= cell;
				//printf("acc:%4x \trep:%4x \tcell: %4x \tj:%d \tpos: %d\n",
				//		accumulator, repeater, cell, j, pos);
			}
			repeater = ~repeater & MAXMASK;
			
			// By the nature of sudoku, if multiple bits all appear only once
			// within a single cell, the puzzle is invalid. If repeater is true,
			// it's guaranteed to have just a single bit set. (Assuming there's
			// no data corruption somewhere.)
			if (repeater) {
				bitboard hidden; int pos;
					// find exact position of hidden value
				for (j=0; j<WIDTH; j++) {
					pos = hiddenLookup[type][i][j]; // exit loop when non-zero value found
					if ((hidden = repeater&getFastMaskYY(pos))) {break;}
				}
				//printf("%x: cell %d of constraint %d of type %d\n", hidden, i, j, type);
				// assign
				grid[pos] = hidden;	
				impossibleYY(pos, hidden);
				found++;
			}
		}
	}
	return found;
}
	// Q: Is the use of getFastMask optimal? Bring back pgrid[]?
#endif

#ifdef DO_NAKED_SINGLE_REDUCTION
inline int detectNakedSinglesYY() {
	int i, found=0;
	// Naked single detection.
	// Find bitmasks with only one bit - lookup table 
	// produces a very minor increase in speed. For 16 bit
	// lookups, using an 8-bit lookup table twice may be a
	// good idea.
	for (i=0; i<GRID_SIZE; i++) {
		if (!grid[i]) {
			bitboard mask = getFastMaskYY(i);
			if ( (((mask&0xff00)==0) && single[mask&0xff]) ||  (((mask&0x00ff)==0) && (single[(mask>>8)/*&0xff*/]))   ) {
				grid[i]=mask;
				impossibleYY(i, mask);
				found++;
				//printf("# %d:%d %x\n", i%WIDTH, i/WIDTH, mask);
			}
		}
	}
	return found;
}
#endif

void inline bruteSolveYY()
{
	
	// There's three 9-entry arrays that show 9-bit possibility masks for rows,columns,
	// and boxes. Getting a possibility mask for a certain cell is then just a matter of
	// ANDing the three arrays together at relevant positions.
	initMasksYY();

	int i;
	//int j;

	// Simplify grid via logic before moving on to a brute force guessing algorithm.
	int found=-1;
	while (found) {
		int nfound=0, hfound=0, reloop=0;
		if (found!=-1) {reloop=1;}
		#ifdef DO_NAKED_SINGLE_REDUCTION
		nfound = detectNakedSinglesYY();
		#endif
		#ifdef DO_HIDDEN_SINGLE_REDUCTION
		hfound = detectHiddenSinglesYY();
		#endif
		found = nfound+hfound;
		#ifdef DEBUG
		if (found) {logit("%dn %dh found%s\n", nfound, hfound, (reloop)?" on extra pass":"");}
		#endif
	}
	
	// set the modifiable[] boolean array
	for (i=0; i<GRID_SIZE; i++) {
		// 0=unknown
		// 1=allready known, can only be one value
		//
		// looping over (!modifiable[p]) is safer than over (pgrid[p])
		// since negative values of p will cause seg faults instead of 
		// infinite mystery loops
		if (grid[i]) {modifiable[i]=0;} else {modifiable[i]=1;}
		
		//logit("%x\t", pgrid[i]); // possibility log
		//if (!((i+1)%WIDTH)) {logit("\n");}
	}


	

	int p=0; // position pointer in the main solving grid
	while (!modifiable[p]) {p++;} // skip any knowns

	// Everything before this takes a microscopic amount of time. Optimizing single detection
	// will shave a few miliseconds off a minute-long run, 
	// TODO: implement block hidden single checking
	//		 cram in single testing into main loop?

	// Here comes the main loop. 99% of the processing happends here.

	// loop until algorithm reaches last cell
	unsigned int count=0;
	do {
		count++;
		// some useful debug code
	/*	logit("%d\trow", p);
		for(i=0;i<WIDTH;i++) {logit("%5x", rowMask[i]);} logit("\n\tcol");
		for(i=0;i<WIDTH;i++) {logit("%5x", colMask[i]);} logit("\n\tblk");
		for(i=0;i<WIDTH;i++) {logit("%5x", blockMask[i]);} logit("\n");
	*/	//		printf("p=%d v=%x mask=%x pos=%d/%d/%d\n", p, v, mask, p/WIDTH, p%WIDTH, blockNumLookup[p]);
		
		// find the possibility mask for this cell
		bitboard mask = (getFastMaskYY(p));
		bitboard v = grid[p]; // could make this a register, but would that speed things up?
									   // -O3 probably handles it fine allready

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
			if (v) {possibleYY(p, v);} else {v=1;}

			// v&mask is true if v is a valid choice for the cell
			// so, let's find a valid value for v
			//
			// this assumes mask has a valid possibility in it... 
			// it always should though
			while (!(v&mask)) {v = v<<1;}
			// make sure the new value can't be re-used in the same constraints
			impossibleYY(p, v);
			grid[p]=v;
			// skip known cells as usual
			do {p++;} while(!modifiable[p]);
		} else {
			// dead end with no possibilities, let's backtrack
			// 
			// it's not backtracking if you don't undo all the effects
			if (v) {possibleYY(p, v);}
			// since the algorithm counts up, the cell must be zero
			// this also signifies that this cell doesn't affect others in
			// any way shape or form
			grid[p]=0;
			// go down by one, then skip any 'given' cells
			do {p--;} while (!modifiable[p]);
		}
	} while(p<GRID_SIZE);

//	logit("%9u cycles\n", count);

}


void solve16(sudata* data) 
{
	// convert to preferred structure format
	int i;
	//for (i=0; i<WIDTH; i++) {rowMask[i]=MAXMASK; colMask[i]=MAXMASK; blockMask[i]=MAXMASK;}
	for (i=0; i<GRID_SIZE; i++) {
		if (data->grid[i]) {grid[i] = 1 << (data->grid[i]-1);/* impossibleYY(i, grid[i]);*/}
		else {grid[i]=0;}
	}
	
	// check validity but not completion
	if (isValidYY(0))  {/*logit("");*/ data->valid=1;}  // valid puzzle, solving
	else 		{logit("!"); data->valid=0; return;} // not valid, ignoring
	
	// check that puzzle isn't complete
	int complete=1;
	for (i=0; i<GRID_SIZE; i++) {if (grid[i]==0) {complete=0; break;}}
	
	// solve
	if (!complete) {
		bruteSolveYY();

		// check validity AND completion
		if (isValidYY(1))  {logit(".");}
		else 	 	{logit("#");}
	}

	// convert back to generic format
	for (i=0; i<GRID_SIZE; i++) {
		if (!grid[i]) {data->grid[i]=0; continue;} // zero handler
		int j;
		for(j=0; j<WIDTH; j++) {
			if ((grid[i]>>j)&1) {
				data->grid[i]=j+1;
				//if (i%WIDTH==0) {logit("\n");}
				//logit("%2.x ", data->grid[i]);
				break;
			}
		}
	//	printf("%d ", data->grid[i]);
	}
}


