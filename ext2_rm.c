#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "utilities.c"
#include "ext2.h"

unsigned char *disk;

int main(int argc, char **argv) {
    
    int t_idx; //target path inode index
    int s_idx; //source path inode index
    struct ext2_inode * t_inode; //target inode
    int i;

    if (argc != 3){ //error checking for number of argumets
		perror("Usage: <disk_image> <target path>"); //print usage for clarification
		exit(-1);
	}
	int file = open(argv[1], O_RDWR); //open
	//create new mapping, MAP_SHARED for commiting chages back to file, ready for read and write
	disk = mmap(NULL, 128*1024, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0); 
	if(disk == MAP_FAILED){ //mmap returns MAP_FAILED if memory mapping fails
		perror("Memory Mapping Error");
		exit(-1);
	}
	

	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *blgrp = (struct ext2_group_desc *)( disk + 2048);
	//struct ext2_super_block block = *(struct ext2_super_block *)(disk_image + EXT2_BLOCK_SIZE);
	struct path *target_path = divide_path(argv[2]);
	if (target_path->isfile == 0){
		perror("FilePath is Directory");
		return ENOENT;  //appropriate return
	}

	if (locate_directory(target_path) == -1){ //use helper function to see if already existing
		perror("The file does not exist");
		return EEXIST;  //appropriate return 
	}
	
	char *filepath = NULL; //the path of the target file
    char *filename = NULL; //the name of the image file
	filename = return_last_entry(argv[2]);//get the name of the file and the target path
	filepath = return_path(argv[2]);
   
    struct path *source_path = NULL;

	if(filepath == NULL){
		s_idx = EXT2_ROOT_INO;
	}else{
		source_path = divide_path(filepath);
		if(strlen(filepath) == 0 || source_path == NULL){ //check if length of filepath is 0, should not be
		perror("Target Filepath Error");
		return ENOENT; //appropriate return
	   }
	   s_idx = locate_directory(source_path);
	}
	
	
	 //get source file inode index
	t_idx = locate_directory(target_path); // get target file inode index
	t_inode = find_inode(t_idx);  //get the inode for the target
	if(s_idx < 1){ //error checking for s_idx
		perror("Parent Not Found");
		return ENOENT;
	}

	if(t_idx > 0){
		if((t_inode->i_mode & EXT2_S_IFDIR)){ //check the file mode for invalid filepath
			perror("Invalid FilePath");	//error checking
			return ENOENT;
		}
	}
	
	remove_dir_entry(s_idx, filename);

	if((t_inode->i_links_count) == 0 && t_inode->i_dtime > 0){
		unset_bitmap('i', t_idx); //delete inode using helper function
		int num_blocks = t_inode->i_blocks / 2;
		sb->s_free_inodes_count++;
		blgrp->bg_free_inodes_count++;

        for(i = 0; i < num_blocks; i++){    
            int block = t_inode->i_block[i];
            unset_bitmap('b', block); 
            sb->s_free_blocks_count++;
		    blgrp->bg_free_blocks_count++;
  
        }
	}else{		
		t_inode->i_dtime = 10;
	}
	
	//free allocated variables
	free(filepath); 
	free(filename);
	return 1;			//DO ERROR CHECKS FOR FILEPATH DIVIDE PATH
}
