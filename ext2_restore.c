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


int main(int argc, char **argv) {
    int i;
    
    // check argument nc
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> < target file path>\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

    int arg_len = strlen(argv[2]);
    char *filename = NULL;
    char *parent_path= NULL;
    if(argv[2][arg_len] == '/'){
        perror(" cannot restore directories");
        exit(1);
    }else{
        remove_last_entry(argv[2],filename, parent_path);
    }
    int parent_idx;
    if(parent_path == NULL){
        parent_idx = 2;
    }else{
        struct path *parent_pt = divide_path(parent_path);
        parent_idx = locate_directory(parent_pt);
    }

    if(parent_idx < 1){
        perror("Cannot find parent file");
        exit(ENOENT);
    }

    struct ext2_inode *parent_inode = find_inode(parent_idx);
    if( parent_inode->i_mode & EXT2_S_IFREG || parent_inode->i_mode & EXT2_S_IFLNK ){
        perror(" Parent is file is not a direcotry ");
        exit(ENOENT);
    }
    if( filename == NULL){
        perror("ERROR READIN PATH");
        exit(ENOENT);
    }
    int dir_size = sizeof(struct ext2_dir_entry) + strlen(filename);
    int num_blocks = parent_inode->i_blocks /2;

    for( i = 0; i < num_blocks; i++){

        if(parent_inode->i_block[i]){
           struct ext2_dir_entry *dir = (struct ext2_dir_entry*) ( disk + EXT2_BLOCK_SIZE* parent_inode->i_block[i]);
           int size = 0;

           while (size < EXT2_BLOCK_SIZE){
               size += dir->rec_len;
               int act_size = sizeof(struct ext2_dir_entry) + dir->name_len;

               if((dir->rec_len - act_size) == dir_size){
                  int ret = check_restore(dir->inode);
                  if(ret == ENOENT){
                     return ENOENT;
                  }
                  dir->rec_len = act_size;
                  dir = (void *) dir + dir->rec_len;
                  dir->rec_len = dir_size;
                  
                  return ret;
               }
               if((EXT2_BLOCK_SIZE - dir->rec_len) == dir_size){
                  int ret = check_restore(dir->inode);
                  if(ret == ENOENT){
                    return ENOENT;
                  }
                  dir->rec_len = act_size;
                  dir = (void *) dir + dir->rec_len;
                  dir->rec_len = EXT2_BLOCK_SIZE - (EXT2_BLOCK_SIZE - size - act_size);                 
                  return ret;
               }              
               dir = (void *) dir + dir->rec_len;
           }

        }

    }

    for( i = sb->s_first_data_block ; i <  BLOCKS_COUNT ; i++){
         struct ext2_dir_entry *dir = (struct ext2_dir_entry*) ( disk + EXT2_BLOCK_SIZE* i);

         if(strncmp(dir->name,filename, strlen(filename)) == 0){
            int ret = check_restore(dir->inode);
            if(ret == ENOENT){
                return ENOENT;
            }
            insert_dir_entry( parent_idx,dir);
         }

    }
    return 0;
}