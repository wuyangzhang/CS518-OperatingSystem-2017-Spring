#include "inode.h"
#include <string.h>
#include "params.h"
#include "block.h"
#include <errno.h>

uint32_t findFreeInode(){
    char inodeBitMap[BLOCK_SIZE];
    block_read(SFS_INODE_BITMAP_BLKS, inodeBitMap);
    uint32_t i;
    for(i = 0; i < BLOCK_SIZE; i++){
        if(inodeBitMap[i] == 1){
            log_msg("Find free inode #%d!\n",i);
            return i;
        }
    }
    log_msg("No Free Inode!\n");
    return 0;
}

uint32_t create_inode_real(sfs_inode_t *inode, char *name, uint8_t mode){
    
    
    // Confirm if the inode is a directory. 
    if(inode->flag!=SFS_FLAG_DIR && inode->flag!=SFS_FLAG_LINK){
        log_msg("This inode is not a directory or link!\n");
        return 0;
    }
    
    // Check if the inode needs to link to another inode
    if(inode->num_link == SFS_PTR_NUM_PER_INODE - 1){
        log_msg("The inode' link number is %d now! Need to link to a new inode.\n",inode->num_link);
        uint32_t inode_new_num = findFreeInode();
        sfs_inode_t inode_new = {
            .iid = inode_new_num,
            .flag = SFS_FLAG_LINK,
            .num_link = 0,
            .size = 0,
            .nblocks = 0,
            .accessed_time = time(NULL),
            .modified_time = time(NULL),
            .inode_modified_time = time(NULL)
        };
        // Update inode information if change is successful
        if(create_inode_real(&inode_new, name, mode)){
            inode->num_link = SFS_PTR_NUM_PER_INODE;
            inode->blk_ptrs[SFS_PTR_NUM_PER_INODE - 1] = inode_new_num;
            inode->nblocks += 1;
            inode->accessed_time = inode->modified_time = inode->inode_modified_time = time(NULL);
            update_inode_data(inode->iid,inode);
            return 1;
        }else{
            inode->accessed_time = time(NULL);
            update_inode_data(inode->iid,inode);
            return 0;
        }
    }
    // Check if the inode is already full
    if(inode->num_link == SFS_PTR_NUM_PER_INODE){
        log_msg("The inode' link number is %d now! Need to go the the next level.\n",inode->num_link);
        sfs_inode_t inode_new;
        get_inodes(inode->blk_ptrs[SFS_PTR_NUM_PER_INODE - 1], &inode_new);
        
        // Update inode information if change is successful
        if(create_inode_real(&inode_new, name, mode)){
            inode->nblocks += 1;
            inode->accessed_time = inode->modified_time = inode->inode_modified_time = time(NULL);
            update_inode_data(inode->iid,inode);
            return 1;
        }else{
            inode->accessed_time = time(NULL);
            update_inode_data(inode->iid,inode);
            return 0;
        }
    }
    
    // Space is enough in this inode, just link the create the new inode here.
    if(inode->num_link < SFS_PTR_NUM_PER_INODE - 1){
        log_msg("The inode' link number is %d now! Just link the create the new inode here.\n",inode->num_link);
        update_inode_bitmap(inode->iid,0);
        uint32_t inode_new_num = findFreeInode();
        sfs_inode_t inode_new = {
            .iid = inode_new_num,
            .flag = mode,
            .num_link = 0,
            .size = 0,
            .nblocks = 0,
            .accessed_time = time(NULL),
            .modified_time = time(NULL),
            .inode_modified_time = time(NULL)
        };
        
        inode->num_link += 1;
        log_msg("Use link %d of Inode %d now.\n",inode->num_link,inode->iid);
        inode->blk_ptrs[inode->num_link - 1] = inode_new_num;
        inode->nblocks += 1;
        memcpy(&inode->ptr_name[inode->num_link - 1], name, SFS_PTR_NAME_MAX_LENGTH);
        inode->accessed_time = inode->modified_time = inode->inode_modified_time = time(NULL);
        update_inode_data(inode->iid,inode);
        update_inode_data(inode_new.iid, &inode_new);
        update_inode_bitmap(inode_new.iid,0);
        return 1;
    }
 
    return 0;
}

uint32_t create_inode(const char *path, uint32_t inode_num, uint8_t mode) {
   
    log_msg("Inode Number: %d\nFile Name: %s\n",inode_num,name);
//    log_msg("File Path: %s\n",path);
    path += 1;
    log_msg("File Path: %s\n",path);
    sfs_inode_t inode;
    get_inodes(inode_num, &inode);
    
    return create_inode_real(&inode,path,mode);
}

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
        uint32_t blockNum = SFS_INODE + inode_no / (BLOCK_SIZE/SFS_INODE_SIZE);
        uint32_t offset = inode_no % (BLOCK_SIZE/SFS_INODE_SIZE);
        //s2: read this block
        char block[BLOCK_SIZE];
        block_read(blockNum, block);
        //s3: revise the partial block
        memcpy(block + offset * SFS_INODE_SIZE, inode, sizeof(sfs_inode_t));
	//s5: write back
  	block_write(blockNum,block);
}

void update_block_data(uint32_t datablock_no, char* buffer){
  uint32_t blockNum = SFS_INODE + SFS_INODE_BLKS_NUM + datablock_no;
    
}

uint32_t path_ino_mapping(uint32_t ino_num, const char *path) {
        //initilize
        char* token;
        sfs_inode_t tmp;
        //split first folder name
        token = strtok(path, "/");
        log_msg("\n	token %s", token);
//	while(token != NULL){
        //get the current inode, look for its "token" named link
        get_inode(ino_num, &tmp);
//        log_msg("\n     %d", );
        int i;
        if(token == NULL){
		return 0;
	}
	for(i = 0; i < tmp.num_link; i++){
                if(strcmp(tmp.ptr_name[i], token)){
                        //get the expected "token named inode num"
                        log_msg("->%d", ino_num);
                        ino_num = tmp.blk_ptrs[i];
                        break;
                }
         }
         if(i == tmp.num_link){
                log_msg("\n     invalid path %s", path);
                return SFS_INODES_NUM + 1;
         }
                //get next "token"(folder name)
//                token = strtok(NULL, "/");
//        	log_msg("token %s", token);
//        }
        return ino_num;
}

void get_inode(uint32_t ino, sfs_inode_t *inode) {
  	//s1: find block that this inode belongs to
	uint32_t blockNum = SFS_INODE + ino / (BLOCK_SIZE/SFS_INODE_SIZE);
	uint32_t offset = ino % (BLOCK_SIZE/SFS_INODE_SIZE);
  	//s2: read this block
  	char block[BLOCK_SIZE];
  	block_read(blockNum, block);
  	//s3: extract the expected inode from block
  	memcpy(inode, block + offset * SFS_INODE_SIZE, SFS_INODE_SIZE);
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
//	statbuf->st_dev = 0;
//	statbuf->st_ino = inode->iid;
	memset(statbuf, 0, sizeof(struct stat));
//	switch(inode->flag){
//		case SFS_FLAG_DIR:
//			statbuf->st_mode = ;}
	statbuf->st_mode = inode->mode;
	statbuf->st_nlink = inode->num_link;
	statbuf->st_size = inode->size;
//	statbuf->st_blksize = BLOCK_SIZE;
	statbuf->st_blocks = inode->nblocks;
	statbuf->st_atime = inode->accessed_time;
	statbuf->st_mtime = inode->modified_time;
	statbuf->st_ctime = inode->inode_modified_time;
}

int isInodeFree(uint32_t inode_no) {
  char inodeBitMap[BLOCK_SIZE];
  block_read(SFS_INODE_BITMAP_BLKS, inodeBitMap);
  if(inodeBitMap[inode_no] == 1){
    return 1;
  }else{
    return 0;
  }
  
}

uint32_t get_ino() {
}

void free_block_no(uint32_t b_no) {
}

uint32_t get_block_no() {
}

void create_dentry(const char *name, sfs_inode_t *inode, uint32_t ino_parent) {
}

void remove_dentry(sfs_inode_t *inode, uint32_t ino_parent) {
}
