#include <stdio.h>
#include <string.h>
#include "dump.h"

typedef unsigned int uint;
typedef unsigned short ushort;
#define SIZE 3
#define GRID_SIZE SIZE*SIZE*SIZE*SIZE

int read(char* string)
{
	fgets(string, 257, stdin);
	//printf("Read: %s\n", input);
	if (strlen(string)<=GRID_SIZE) {	
		printf("Exiting.\n");
		return 0;
	}
	return 1;

}

int load(char* string, uint* grid)
{
	int i;
	for (i=0; i<GRID_SIZE; i++) {
		if (string[i]=='.') {grid[i] = 0x01FF0000;} // 
		else {
			ushort num = 0x1 << (grid[i]-49);
			printf("%d\n", grid[i]-49);
			grid[i] = num;
		}
	}
	return 0;
}

int unload(char* string, uint* grid)
{
	int i,j;
	for (i=0; i<GRID_SIZE; i++) {
		string[i]=49;
		for (j=0; j<16; j++) {
			if (grid[i]&1) {string[i]++;}
		}
	}
}
