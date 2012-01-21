#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <unistd.h> // lseek()
#include <stdlib.h> // exit()
#include <string.h>
#include "datum.h"
#include "global.h"
#include "log.h"

void inline printGrid(sudata* p)
{
#ifdef DEBUG_PRINTSUDOKU
	debug("\t\t");
	int sSize = (p->width==9)?81:256;
	int i;
	for (i=0; i<sSize; i++) {
		debug("%c ", (p->grid[i]<=9)?(p->grid[i]+48):(p->grid[i]+48+39));
		if ((i+1)%p->width==0) {debug("\n\t\t");}
	}
	debug("\n");
#endif
}

// used for rotating the puzzle it's loaded
char tempgrid[SIZE_4x];

// I thought 'input validation' implied testing that the puzzle is valid before
// trying to slove it. Turns out it means validating input data - this function
// isn't too great at it.
//
// Anything between a valid start statement and a an 'en' statement currently has to be
// spot on. This is very easy to do since the format is so simple, but could cause
// show stopping problems if gotten wrong.
//
// buff is a pointer to a memory mapped file in kernel address space: it's opened
// as read only, so a copy should be made if it has to be edited (strtok() for example)
// (currently the loading process does not edit the input buffer)
sudata* parseInputBuffer(const char* buff, int size)
{
	debug("Reading %d byte input file.\n", size);
	const char* p = buff;

	int width;
	
	sudata* head=(sudata*)0xDEADBEEF;
	sudata* last=0;

	//sudata* data = (sudata*)calloc(1, sizeof(sudata));
	while( (p=strstr(p, "start"))!=NULL ) {
		// cut off 'start'
		p += 5; 
		// cut off number and \n
		if (p[0]=='9') {p+=2; width=9;} else
		if ((p[0]=='1') && (p[1]=='6')) {p+=3; width=16;} 
		else {
			logit("start header with wrong number found, ignoring section.\n");
			continue;
		}

		// We got this far, probably ain't bogus... Insert into list.
		sudata* data = (sudata*)calloc(1, sizeof(sudata));
		if (last==0) {head=data; last=head;} else {last->next=data; last=data;}
		// Any errors further than this will get processed.

		data->width=width;
		//debug("\tWidth %d.\n", width);
		
		int cx=0; // x-centroid
		int cy=0; // y-centroid
		int cn=0; // number of values

		// pop off x y val triplets until "end"
		while (!((p[0]=='e') && (p[1]=='n'))) {
			int x,y,v;
			// Load 16x16 puzzles in decimal... However if hex is used,
			// load both types of puzzles in the same (fast) manner.
			#ifndef FILE_FORMAT_HEX
			if (width==16) {
				/*#define POPVAL(a) if ((p[0]=='1')&&(p[1]!=' ')) \
							{a = p[1]-38; p+=3;} else \
							{a = p[0]-48; p+=2;}
				POPVAL(x) POPVAL(y) POPVAL(v) 	*/
				// ...maybe this isn't a good idea, let's use sscanf instead
				
				// scanf is slow! 1.5 seconds to load ~100 full (9x9) solutions...
				// I need to make sure it never gets used to load 9x9 puzzles.
				//
				// Of course, in most guess-heavy puzzles the backtracking 
				// algorithm is the main source of slowdown... But for logic 
				// solvable ones? God only knows.
				sscanf(p, "%d %d %d\n", &x, &y, &v);
				//printf("%d %d %d\n", x, y, v);
				p = strchr(p,'\n')+1;
			} else {
			#endif
				// hex format reading - guess that's not what we're doing?
				v = p[4]-48;
				x = p[0]-48;
				y = p[2]-48;
				if (v>9) {v-=39;}
				if (x>9) {x-=39;}
				if (y>9) {y-=39;}
				p += 6;
			#ifndef FILE_FORMAT_HEX
			}
			#endif



			cx+=x; cy+=y; cn++; // centroid calculation
			//data->grid[p[0]-48+(p[2]-48)*width] = p[4]-48;
			data->grid[x+y*width] = v;

			//debug("\t[%d:%d] = %d\n", p[0]-48, p[2]-48, p[4]-48);
		}
		printGrid(data);
// TODO not size-safe!
		#ifdef DO_CENTROID_FLIP
if (width==9) {		
		// find centroid
		float cxf = (float)cx/cn; float cyf = (float)cy/cn;
		cxf -= 4; cyf -= 4; //cxf=1; cyf=0;
		//debug("centroid at %lf %lf\n", cxf, cyf);
		// flip on x axis if 'right' side is bigger
		if (cxf<=0) 	{data->rotmat[0]= 1; data->rotmat[1]=0;}
		else 		{data->rotmat[0]=-1; data->rotmat[1]=0;}
		// flip on y axis if higher-value side bigger
		if (cyf<=0) 	{data->rotmat[2]=0; data->rotmat[3]= 1;}
		else 		{data->rotmat[2]=0; data->rotmat[3]=-1;}

		int arrSize = width*width;
		memcpy(tempgrid, data->grid,arrSize);
		int i;
		//debug("rotmat: %d %d %d %d\n", data->rotmat[0], data->rotmat[1], data->rotmat[2], data->rotmat[3]);
		for (i=0; i<arrSize; i++) {
			// origin at center!
			int tx=i%width-4;
			int ty=i/width-4;
			// origin in corner!
			int x = tx*data->rotmat[0] + ty*data->rotmat[1] +4;
			int y = tx*data->rotmat[2] + ty*data->rotmat[3] +4;
			data->grid[x+y*width] = tempgrid[i];
		}
}
		#endif

		// this may be rotated - don't show in order to not confuse user
		//printGrid(data);


	}

	return head;
	
}

// Is loading the file a significant portion of the work? No.
// Does it need to be fancy? No.
// Is memory mapping cool? Hell yes!
#ifdef PARALLEL
void* share; int sharef; off_t sharesize;
#endif
sudata* load(const char* filename)
{	
	int inf; void* in;

	if ( (inf=open(filename, O_RDONLY)) == -1 ) {
		logit("Error opening input file for reading, exiting.\n");
		exit(1);
	}
	
	off_t filesize = lseek(inf, 0, SEEK_END);
	if ( (in = mmap(0, filesize, PROT_READ, MAP_PRIVATE, inf, 0)) == (void*)-1 ) {
		logit("Memory mapping input file failed, exiting.\n");
		exit(2);
	}

	sudata* ret = parseInputBuffer((char*)in, filesize);

	munmap(in, filesize);
	close(inf);

	#ifdef PARALLEL
	int count=1; sudata* p;
	for (p=ret; p->next!=(sudata*)0; p=p->next) {count++;} // find number of puzzles
	
	sharesize = sizeof(sudata)*count;
	logit("%d puzzles READ!", count);

	if ( (sharef=open("/dev/zero", O_RDWR)) == -1) {logit("Failed to open shared memory file, bailing.\n"); exit(1);}
	if ( (share = mmap(0, sharesize, PROT_READ | PROT_WRITE, MAP_SHARED, sharef, 0)) == (void*)-1) {
		logit("Failed to mmap shared memory, bailing.\n"); exit(1);
	}
	close(sharef);


	p = ret; ret = share; // p is start of old list, ret is start of new list
	void* position = share; // position is our i, in the new list
	sudata* last=(sudata*)0;
	while (p!=(sudata*)0) {
		memcpy(position, p, sizeof(sudata));
		if (last) {last->next = position;}
		last = position;
		position += sizeof(sudata);


		sudata* freeme=p;
		p = p->next;
		free(freeme);
	}
	#endif


	return ret;
}


void saveSudata(sudata* p, FILE* out);

void save(const char* filename, sudata* head)
{
	FILE* fout;

	if (!strcmp(filename, "stdout")) {
		fout = stdout; 
		logit("Printing results to stdout:\n\n");
		logFlush();
	} else if (!(fout = fopen(filename, "w"))) {
		logit("Couldn't open output file %s\n", filename);
		exit(1);
	}

	saveSudata(head, fout);


	fclose(fout);
	#ifdef PARALLEL
	munmap(share, sharesize);
	#endif
}

// start16\n = 8 chars
// a b c\n = 6 chars * 256 cells
// end\n = 4 chars
// --
// 1548 bytes
char outBuffer[2048];

void saveSudata(sudata* p, FILE* out)
{
	//logit("\tWidth %d sudoku.\n", p->width);
	int i;
	int width = p->width;
	int arrSize = width*width;
	#ifdef DO_CENTROID_FLIP
	if (p->width==9) { // TODO: update rotation code for 16x16
	memcpy(tempgrid, p->grid, arrSize);
	for (i=0; i<arrSize; i++) {
		int tx=i%width-4;
		int ty=i/width-4;
		// transpose/inverse of rotmat
		// watch for origin
		int x = tx*p->rotmat[0] + ty*p->rotmat[2] +4;
		int y = tx*p->rotmat[1] + ty*p->rotmat[3] +4;
		p->grid[x+y*width] = tempgrid[i];
	}
	}
	#endif

	
	printGrid(p);

	char* pen = outBuffer; pen[0]='\0';
	pen += sprintf(pen, "start%d\n", width);

	#define NUM2CHAR(a) (a += (a<=9)?(48):(87))
	if (p->valid) for (i=0; i<arrSize; i++) {
		char x = i%width;
		char y = i/width;
		char val = p->grid[i];
		#ifdef FILE_FORMAT_HEX 
		NUM2CHAR(x); NUM2CHAR(y); NUM2CHAR(val);
		pen += sprintf(pen, "%c %c %c\n", x, y, val);
		#else
		pen += sprintf(pen, "%d %d %d\n", x, y, val);
		#endif
	}

	pen += sprintf(pen, "end\n");

	fprintf(out, "%s", outBuffer);

	if (p->next) {saveSudata(p->next, out);} 

	#ifndef PARALLEL	
	free(p);
	#endif
}



