#ifndef __DATA_LIST_H
#define __DATA_LIST_H

#include "list.h"

typedef struct element {
	int id;
	char name[256];
	list_t list;
	list_t list2;
} element_t;


#endif
