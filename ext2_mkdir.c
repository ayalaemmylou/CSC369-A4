#include "ext2.h"
#include "utilities.c"
#include <string.h>
#include <stdio.h>

unsigned char *disk;

int main(int argc, char ** argv){
	//char *disk_image; //to store file we are memory mapping
	char slash = '/'; //used to check the file path
	char *filepath = NULL;  //used to set argv[2] filepath
	char *filename = NULL; //used to set argv[2] filename
	int p_idx;	//the parent index to find inode
	int dir_idx; //create new inode for directory using this
	struct ext2_inode * p_inode; //the parent inode
	struct ext2_inode *dir_inode; //the new directory inode
	struct ext2_dir_entry dir_entry; //the directory entry
	struct ext2_dir_entry self_pointer; //this represents the . which points to current file
	struct ext2_dir_entry parent_pointer; //this represents the .. which points to parent
	int i; //for loop

	if (argc != 3){		//error checking for number of argumets
		perror("Usage: <file name> <file path>\n");  //print usage for clarification
		exit(-1);
	}
	int file = open(argv[1], O_RDWR);	//open the image file for read or write
	if (file < 0){	//error check for file opening
		perror("File Opening Error");
		exit(-1);
	}
	//create new mapping, MAP_SHARED for commiting chages back to file, ready for read and write
	disk = mmap(NULL, 128*1024, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0); 
	if(disk == MAP_FAILED){ //mmap returns MAP_FAILED if memory mapping fails
		perror("Memory Mapping Error");
		exit(-1);
	}
	
	if(strncmp(argv[2], &slash, 1) != 0){	//error checking file path, returns 0 if equal
		perror("File Path Error");
		exit(-1);
	}
	filename = return_last_entry(argv[2]);
	filepath = return_path(argv[2]);//split the name from the path of argv[2]
    struct path *parent;

	if(filepath == NULL){
		p_idx = 2;
	}else{
		parent = divide_path(filepath);	//dividing path to get the index
		if ((p_idx = locate_directory(parent)) == -1){  //get the index of the filepath
			perror("Invalid FilePath");	//error checking
			return ENOENT;
		}
    }
	p_inode = find_inode(p_idx);
	if((p_inode->i_mode & EXT2_S_IFREG)){ //check the file mode for invalid filepath
		perror("Invalid FilePath");	//error checking
		return ENOENT;
	}
	if ((dir_idx = new_inode()) == -1){  //create new inode and error check
		perror("Error creating new node");
		exit(-1);
	}
	dir_inode = find_inode(dir_idx);
	dir_inode->i_links_count = 1; //set the links
	dir_inode->i_mode = 1; // set to directory
	dir_inode->i_size = 10; //set the size
	dir_inode->i_blocks = 0; // set the number of blocks being used
	for(i = 0; i < 15; i++){  //set all blocks of inode to 0, 15 blocks
		dir_inode->i_block[i] = 0; 
	}
	//set everything needed for the new directory
	strncpy(dir_entry.name, filename, strlen(filename));
	dir_entry.name[strlen(filename)] = '\0';
	dir_entry.name_len = strlen(filename);
	dir_entry.rec_len = 0;
	dir_entry.inode = dir_idx; // set the inode to the directory index
	dir_entry.file_type = EXT2_FT_DIR;  //set the file type to directory
	printf("hgvghg\n");
	insert_dir_entry(p_idx, &dir_entry); //making the new directory entry
	self_pointer.inode = dir_idx; //setting the self pointer to the current dir we made
	parent_pointer.inode = p_idx; //setting the parent_pointer to the directory our dir is in
	strncpy(self_pointer.name, ".", 1); //copy the name of self
	(self_pointer.name)[1] = '\0';
	strncpy(parent_pointer.name, "..", 2); //copy the name of the parent
	(parent_pointer.name)[2] = '\0';
	self_pointer.name_len = 1; //since just .
	parent_pointer.name_len = 2; // since just ..
	self_pointer.rec_len = 0; 
	parent_pointer.rec_len = 0; 
	self_pointer.file_type = EXT2_FT_DIR;	//set as directory
	parent_pointer.file_type = EXT2_FT_DIR; //set as directory
	printf("ghvggh\n");
	insert_dir_entry(dir_idx, &self_pointer); //add the entries to dir_idx
	printf("dxdxt\n");
	insert_dir_entry(dir_idx, &parent_pointer); //add the entries to dir_idx

	//free everything malloc'd
	free_path(parent); //allocated in helper function
	free(filepath);
	free(filename);
	return 1;
}
