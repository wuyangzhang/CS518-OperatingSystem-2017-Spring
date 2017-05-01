/*
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  There are a couple of symbols that need to be #defined before
  #including all the headers.
*/

#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500

// maintain bbfs state in here
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include "list.h"

typedef struct {
	int id;
	list_t node;
} sfs_free_list;

struct sfs_state {
    FILE *logfile;
    char *diskfile;

    sfs_free_list* state_inodes; // Array of list nodes for all inodes state info
    sfs_free_list* state_data_blocks; // Array of data block nodes for all data blocks

    list_t* free_inodes;
    list_t* free_data_blocks;

    uint32_t ino_root;
};

#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)

#endif
