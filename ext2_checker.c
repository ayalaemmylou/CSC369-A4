#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include "utilities.c"

unsigned char *disk;

int main(int argc, char**argv){
	int num_blocks; //used to traverse blocks
	int num_fixes = 0; // initialize total num of fixes
  int i,k,n,j;

	//error check for number of arguments
	if(argc != 2){
		perror("usage: <disk_image>"); //print usage
		exit(-1);
	}
	int fd = open(argv[1], O_RDWR);	//open disk image for read/write

	//create new mapping, MAP_SHARED for commiting chages back to file, ready for read and write
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);	//memory mappiing
    if(disk == MAP_FAILED) { //mmap returns MAP_FAILED if memory mapping fails
        perror("mmap");
        exit(1);
    }
    struct ext2_super_block *block = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    //group descriptor struct used for checking group blocks
	struct ext2_group_desc *blgrp = (struct ext2_group_desc *)( disk + 2048);
	
    //checking the free inodes
    int num_free = 0;
    for ( j = 1; j < 32; j++){
    	if(!(check_bitmap('i', j))){
    		num_free++;
    	}
    }
    //checking if the number of free inodes is equal to the block free inodes count
    if(num_free != block->s_free_inodes_count){ //if not sets it, checker part A
    	printf("Fixed: superblock's free inodes counter was off by %d compared to the bitmap\n", abs(block->s_free_inodes_count - num_free));
    	num_fixes++; //increment total number of fixes
      block->s_free_inodes_count = num_free;
    }
    //checking if the number of free inodes is equal to the group blocks free inodes count
    if(num_free != blgrp->bg_free_inodes_count){ //if not sets it, checker part A
    	printf("Fixed: block group's free inodes counter was off by %d compared to the bitmap\n", abs(blgrp->bg_free_inodes_count - num_free));
   		num_fixes++; //increment total number of fixes
      blgrp->bg_free_inodes_count = num_free;
    }
    //checking the free blocks
    int num_free_blocks = 0;
    for ( k = 1; k < 128; k++){
    	if(!(check_bitmap('b', k))){
    		num_free_blocks++;
    	}
    }
    //checking if the number of free blocks is equal to the blocks free blocks count
    if(num_free_blocks != block->s_free_blocks_count){ //if not sets it, checker part A
    	printf("Fixed: superblock's free blocks counter was off by %d compared to the bitmap\n", abs(block->s_free_blocks_count - num_free_blocks));
    	num_fixes++; //increment total number of fixes
      block->s_free_blocks_count = num_free_blocks;
    }
    //checking if the number of free blocks is equal to block groups free blocks count
    if(num_free_blocks != blgrp->bg_free_blocks_count){ //if not sets it, checker part A
    	printf("Fixed: block group's free blocks counter was off by %d compared to the bitmap\n", abs(blgrp->bg_free_blocks_count - num_free_blocks));
    	num_fixes++; //increment total number of fixes
      blgrp->bg_free_blocks_count = num_free_blocks;
    }
    //b
    //d
    int num_blocks_fixed;
    for( i = 1; i < 32; i++){
      printf("%d\n",i );
      
    	if(i > 2 && i < EXT2_GOOD_OLD_FIRST_INO){
    		continue;
    	}
      if(!check_bitmap('i', i)){
        printf(".;.;.'.'.;\n");
        continue;
      }
      printf("scdcsd...............\n");
    	struct ext2_inode * inode = find_inode(i);
        if(inode->i_size == 0 ){ //check if the size in bytes is 0
            continue; //go to next forloop spin if that is the case
        }
        num_blocks_fixed = 0;
        num_blocks = inode->i_blocks/2;
        printf("....%d\n.....", num_blocks);

    	for( n = 0; n < num_blocks; n++){
        printf(".....%d\n", n );
        if( n > 10){
          break;
        }
    		if(inode->i_block[n] < 1){
    			continue;
    		}
        
    		if(inode->i_mode & EXT2_S_IFREG){ //check if regular file
          printf("sdsd\n" );
    			if(!(check_bitmap('b', inode->i_block[n]))){ //returns what the specific index is in the bitmap, inode or block
    				set_bitmap('b', inode->i_block[n]); //if not block, set bitmap
    				blgrp->bg_free_blocks_count--; 
    				block->s_free_blocks_count--;
    				num_blocks_fixed++; //increment number of blocks fixed
    				num_fixes++; //increment total number of fixes
    				continue; //go through next loop
    			}
    		}
        if(inode->i_mode & EXT2_S_IFLNK){
          continue;
        }

    		//directory struct we are going to use for checking
    		struct ext2_dir_entry *dir = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode->i_block[n]);
    		int size = 0;
    		if(!(check_bitmap('b', inode->i_block[n]))){ //returns what the specific index is in the bitmap, inode or block
    			set_bitmap('b', inode->i_block[n]); //if not block, set bitmap
    			blgrp->bg_free_blocks_count--;
    			block->s_free_blocks_count--;
    			num_blocks_fixed++; //increment number of blocks fixed
    			num_fixes++; //increment number of fixes
    			}
        		while( size < EXT2_BLOCK_SIZE){ //while we are traversing blocks
          			size += dir->rec_len; 
          			//get the directory inode
          			struct ext2_inode *dir_inode = find_inode(dir->inode);
          			//check if the file type is not directory and inode is directory
          			if (dir->file_type != EXT2_FT_DIR && dir_inode->i_mode & EXT2_S_IFDIR) {
                 		dir->file_type = EXT2_FT_DIR; //if true set them, checker part B
                 		printf("Fixed: Entry type vs inode mismatch: inode[%d]\n", dir->inode);
                 		num_fixes++; //increment total number of fixes
          			}
          			//check if the file type is  not a regular file and inode is regular file
          			if(dir->file_type != EXT2_FT_REG_FILE && dir_inode->i_mode & EXT2_S_IFREG){
          				dir->file_type = EXT2_FT_REG_FILE; //if true set them, checker part B
          				printf("Fixed: Entry type vs inode mismatch: inode[%d]\n", dir->inode);
          				num_fixes++; //increment total number of fixes
          			}
          			//check if the file type is not symbolic link and the inode is symbolic link
          			if(dir->file_type != EXT2_FT_SYMLINK && dir_inode->i_mode & EXT2_S_IFLNK){
          				dir->file_type = EXT2_FT_SYMLINK; //if true set them, checker part B
          				printf("Fixed: Entry type vs inode mismatch: inode[%d]\n", dir->inode);
          				num_fixes++; //increment total number of fixes
          			}
          			//check if the d_time is not 0
          			if(dir_inode->i_dtime != 0){
          				dir_inode->i_dtime = 0; //set it back to 0, checker part D
          				printf("Fixed: valid inode marked for deletion: [%d]\n", dir->inode);
          				num_fixes++; //increment totatl number of fixes
          			}
          			//check the inodes in use
          			if(!(check_bitmap('i', dir->inode))){
          				set_bitmap('i', dir->inode); //set the bitmap if not if, checker part E
          				printf("Fixed: inode [%d] not marked as in-use\n", dir->inode);
          				num_fixes++; //increment total number of fixes
          			}
          			dir = (void *) dir + dir->rec_len;
                
     			   }
             printf("sdcsdc\n");
      }
        //print statement for checker part E
      printf("erefef\n");
        if(num_blocks_fixed > 0){
        	printf("Fixed: %d in-use data blocks not marked in bitmap for inode: [%d]\n", num_blocks_fixed, i);
        }

    }
    //print the number of fixes if there were any inconsitencies
    if(num_fixes == 0){
        printf("No file system inconsistencies detected!\n");
    }else{
        printf("%d file system inconsistencies repaired!\n", num_fixes);
    }
    return 0;
}