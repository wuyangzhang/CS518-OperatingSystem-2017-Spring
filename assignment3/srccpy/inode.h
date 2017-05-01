/*
 * inode.h
 *
 */

#ifndef SRC_INODE_H_
#define SRC_INODE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fuse.h>

#define SFS_FLAG_DIR 0
#define SFS_FLAG_FILE 1
#define SFS_FLAG_LINK 2

#define SFS_DATA_BLKS_NUM 56
//#define SFS_DATA_BLKS_SIZE 512
#define SFS_INODES_NUM 80 // Max number of inodes/files
#define SFS_INODE_SIZE 128 // Size in bytes of inode struct, below mentioned struct should be < 128bytes
#define SFS_INODE_BLKS_NUM (SFS_INODES_NUM / (BLOCK_SIZE / SFS_INODE_SIZE)) // Number of blocks for inodes = 64

#define SFS_INODE_BITMAP_BLKS 1 // Can store 80 inodes
#define SFS_DATA_BITMAP_BLKS (SFS_INODE_BITMAP_BLKS + 1) // Can store 56 blocks
#define SFS_INODE (SFS_DATA_BITMAP_BLKS + 1)
#define SFS_DATA_BLK (SFS_INODE + SFS_INODE_BLKS_NUM)

#define SFS_TOTAL_SIZE ((3 + SFS_INODE_BLKS_NUM + SFS_DATA_BLKS_NUM)*BLOCK_SIZE) //1-superblock 

#define SFS_PTR_NUM_PER_INODE 8
//#define SFS_MAX_LENGTH_FILE_NAME 32
#define SFS_DENTRY_SIZE 64

typedef struct __attribute__((packed)) {
	uint32_t iid;     /* inode number */
	uint8_t flag;	/* type (Dir/file/link)*/
	uint16_t num_link;   /* number of hard links */
//	uint16_t uid;   /* usr id of file owner */
//	uint16_t gid;   /* group number */
	uint32_t size;    /* total size, in bytes */
 	uint32_t nblocks;  /* number of 512B blocks allocated */
 	uint32_t accessed_time;   /* time of last access */
 	uint32_t modified_time;   /* time of last modification */
   	uint32_t inode_modified_time;   /* time of last status change */
	uint32_t blk_ptrs[SFS_PTR_NUM_PER_INODE]; 	/* Size = (32 + ) bytes */
	uint64_t ptr_name[SFS_PTR_NUM_PER_INODE];
} sfs_inode_t;

//typedef struct __attribute__((packed)) {
//	uint32_t inode_number;
//	char name[SFS_MAX_LENGTH_FILE_NAME]; /* File name */
//} sfs_dentry_t;

uint32_t path_ino_mapping(const char* path);

void get_inode(uint32_t ino, sfs_inode_t *inode_data);

uint32_t create_inode(const char *path, mode_t mode);

int remove_inode(const char *path);

int write_inode(sfs_inode_t *inode_data, const char* buffer, int size, int offset);

int read_inode(sfs_inode_t *inode_data, char* buffer, int size, int offset);

void fill_stat_from_ino(const sfs_inode_t* inode, struct stat *statbuf);

//void read_dentries(sfs_inode_t *inode_data, sfs_dentry_t* dentries);

#endif /* SRC_INODE_H_ */
