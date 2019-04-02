/****************************************************/
/* files.h - The file system driver for the kernel. */
/****************************************************/
#ifndef FILES_H
#define FILES_H



#include "types.h"
#include "lib.h"



/* Constants. */
#define MAX_NUM_FS_DENTRIES  63
#define MAX_FILENAME_LENGTH  32
#define FS_PAGE_SIZE         0x1000 // 4kB
#define FS_STATS_SIZE        64



/* 
 * File system statistics format provided at the beginning of
 * the boot block.
 */
typedef struct {
	uint32_t num_dentries;
	uint32_t num_inodes;
	uint32_t num_datablocks;
	uint8_t  reserved[52];
} fs_stats_t;

/*
 * File system directory entry format.
 */
typedef struct {
	int8_t   filename[32];
	uint32_t filetype;
	uint32_t inode;
	uint8_t  reserved[24];
} dentry_t;

/*
 * Inode block format.
 */
typedef struct{
	uint32_t size;
	uint32_t data_blocks[1023];
} inode_t;



/* Opens the file system by calling fs_init. */
int32_t fs_open(uint32_t fs_start, uint32_t fs_end);

/* Close the file system. */
int32_t fs_close(void);

/* Performs a read on the file with name 'fname'. */
int32_t fs_read(const int8_t * fname, uint32_t offset, uint8_t * buf, uint32_t length);

/* Does nothing as our file system is read only. */
int32_t fs_write(void);

/* Loads an executable file into memory and prepares to begin the new process */
int32_t fs_load(const int8_t * fname, uint32_t address);

/* Initializes global variables associated with the file system. */
void fs_init(uint32_t fs_start, uint32_t fs_end);

/* Returns directory entry information from the given name */
int32_t read_dentry_by_name(const uint8_t * fname, dentry_t * dentry);

/* Returns directory entry information from the given index */
int32_t read_dentry_by_index(uint32_t index, dentry_t * dentry);

/* Reads bytes starting from 'offset' in the file with the inode 'inode'. */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length); 

/* Test function for the file system driver.  */		  
void files_test(void);

/* Returns 0 */
int32_t file_open(void);

/* Returns 0 */
int32_t file_close(void);

/* Performs a fs_read. */
int32_t file_read( uint8_t * buf, uint32_t length, const int8_t * fname, uint32_t offset );

/* Returns -1 */
int32_t file_write(void);

/* Returns 0 */
int32_t dir_open(void);

/* Returns 0 */
int32_t dir_close(void);

/* Implements ls. */
int32_t dir_read(uint8_t * buf);

/* Return -1 */
int32_t dir_write(void);



#endif /* FILES_H */
