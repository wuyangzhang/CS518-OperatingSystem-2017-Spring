#include "inode.h"
#include "params.h"
#include "block.h"
#include <errno.h>

void read_dentry_from_block(uint32_t block_id, sfs_dentry_t* dentries, int num_entries);

uint32_t path_2_ino_internal(const char *path, uint32_t ino_parent);

void free_ino(uint32_t ino);

uint32_t get_ino();

void free_block_no(uint32_t b_no);

uint32_t get_block_no();

void update_inode_bitmap(uint32_t inode_no, char ch){
  char inodeBitMap[BLOCK_SIZE];
  block_read(SFS_INODE_BITMAP_BLKS, inodeBitMap);
  inodeBitMap[inode_no] = ch;
  block_write(SFS_INODE_BITMAP_BLKS, inodeBitMap);
}

void update_block_bitmap(uint32_t block_no, char ch){
  char datablockBitMap[BLOCK_SIZE];
  block_read(SFS_DATA_BITMAP_BLKS, datablockBitMap);
  datablockBitMap[block_no] = ch;
  block_write(SFS_DATA_BITMAP_BLKS, datablockBitMap);
}

void update_inode_data(uint32_t inode_no, sfs_inode_t *inode){
  //s1: find block that this inode belongs to
  uint32_t blockNum = SFS_INODE + inode_no /4;
  //s2: read this block
  char block[BLOCK_SIZE];
  memset(block, 0, BLOCK_SIZE);
  block_read(blockNum, block);
  //s3: revise the partial block
  memcpy(inode, block+ (inode_no % 4)*SFS_INODE_SIZE，SFS_INODE_SIZE);
  //s4: write back
  block_write(blockNum,block);
}

void update_block_data(uint32_t datablock_no, char* buffer){
  uint32_t blockNum = SFS_INODE + SFS_INODE_BLKS_NUM + datablock_no;
    
}

uint32_t path_ino(const char *path) {
}

uint32_t path_2_ino_internal(const char *path, uint32_t ino_parent) {
}

void get_inode(uint32_t ino, sfs_inode_t *inode_data) {
}

uint32_t create_inode(const char *path, mode_t mode) {
}

int remove_inode(const char *path) {
}

int write_inode(sfs_inode_t *inode_data, const sfs_inode_t* bufferInode, int size, int offset) {
  
}

int read_inode(sfs_inode_t *inode_data, char* buffer, int size, int offset) {
  
}

void fill_stat_from_ino(const sfs_inode_t* inode, struct stat *statbuf) {
}

void read_dentries(sfs_inode_t *inode_data, sfs_dentry_t* dentries) {
}

void read_dentry_from_block(uint32_t block_id, sfs_dentry_t* dentries, int num_entries) {
}

void free_ino(uint32_t ino) {
}

uint32_t get_ino() {
}

void free_block_no(uint32_t b_no) {
}

uint32_t get_block_no() {
}

void update_inode_bitmap(uint32_t ino, char ch) {
}

void update_block_bitmap(uint32_t bno, char ch) {
}

void update_inode_data(uint32_t ino, sfs_inode_t *inode) {
}

void update_block_data(uint32_t bno, char* buffer) {
}

void create_dentry(const char *name, sfs_inode_t *inode, uint32_t ino_parent) {
}

void remove_dentry(sfs_inode_t *inode, uint32_t ino_parent) {
}
