#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>
#include "utilities.c"


unsigned char *disk;

int main(int argc, char **argv) {
	char* filename = NULL;
	char* path = NULL;
	//check if the number of arguments is correct
	if (argc != 4){
		perror("usage: <image file> <source path> <target path>"); //print usage
		exit(-1);
	}

	int fd = open(argv[1], O_RDWR);
	//create new mapping, MAP_SHARED for commiting chages back to file, ready for read and write
	disk = mmap(NULL, 128*1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
	if(disk == MAP_FAILED){ //mmap returns MAP_FAILED if memory mapping fails
		perror("Memory Mapping Error");
		exit(-1);
	}

	char *link = argv[2]; //set link string as source path
	char *file = argv[3]; //set file string as target path

	//check if the path is correct by comparing lengths
	if( strlen(argv[2]) < 2 ||  strlen(argv[2]) < 2){
		perror("Iconrrent path entered");
		exit(1);
	}
	//put both link and file into path structs
	struct path *file_path = divide_path(file); 
	struct path *exist_link = divide_path(link);
	
	//get the filename and path of the link
	filename = return_last_entry(link);
	path = return_path(link);

	//filename should not be NULL
	if(filename == NULL){
		perror(" invaled link path (source)");
		return ENOENT;
	}
	
	int parent_idx; //parent directory index
	struct path* link_p_path; 
	if(path == NULL){	
		parent_idx = 2; //first reserved block pointer
	}else{
		link_p_path = divide_path(path); 
		parent_idx = locate_directory(link_p_path); //locate the directory of our target path, set as parent index
    }
	int exist_idx = locate_directory(exist_link); //to check if the file already exists in the path
    free_path(link_p_path); //free since no longer needed

	if(exist_idx > 0){ 
		perror("link already exist");
		return EEXIST;
	}
	free_path(exist_link); //free since no longer needed


	if( file_path == NULL){ //check if file not found
		perror("target file not found");
		exit(1);
	}
	
  	//get the index of the link by locating directory of path
    int link_idx = locate_directory(file_path);

	if(link_idx < 1){ //should not be less than 1
		perror("target file not found");
		return ENOENT;
	}

	struct ext2_inode *link_inode = find_inode(link_idx); //find the inode corresponding to the link index
	if(link_inode->i_mode & EXT2_S_IFDIR){ //check if is directory, we do not want it to be
		perror("target file is dir");
		return EISDIR;
	}
     
	//shouldnt be less than 1
    if((parent_idx) < 1){
    	perror("target file not found");
		return ENOENT;
    }
    //set everything needed for the link entry
    struct ext2_dir_entry dir;
    dir.file_type = EXT2_FT_REG_FILE;
    dir.inode = link_idx;
    char *dir_name = dir.name;
    strncpy(dir_name, filename , strlen(filename));
    dir.name[strlen(filename)] = '\0';
    dir.name_len = strlen(filename);
    dir.rec_len = 0;
    //insert the entry
    insert_dir_entry(parent_idx, &dir);
 	
 	//free all allocated files
    free_path(file_path);
    free(path);
    free(filename);
    return 0;

  
}
