#ifndef _DISKRW_H_
#define _DISKRW_H_

#include "datum.h"

// returns pointer to the head of a linked list
sudata* load(const char* filename);

// dumps data to disk
void save(const char* filename, sudata* head);

#endif
