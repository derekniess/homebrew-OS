/*************************************************/
/* syscalls.h - The system call implementations. */
/*************************************************/
#ifndef SYSCALLS_H
#define SYSCALLS_H



#include "files.h"
#include "paging.h"



/*** CONSTANTS ***/
#define		IN_USE			           1
#define		NOT_IN_USE		           0
#define		FILE_TYPE_RTC			   0
#define		FILE_TYPE_DIRECTORY		   1
#define		FILE_TYPE_REGULAR_FILE	   2
#define     PROGRAM_LOAD_ADDR          0x08048000
#define     ENTRY_POINT_OFFSET         24
#define     INITIAL_SHELLS_BITMASK     0x70
#define     INITIAL_KERNEL_STACK_SIZE  60


/*** STRUCTS ***/
/* Explanation: 
 * This is the file descriptor used in the fds array for each process's PCB
 *    jumptable -- A pointer to a file operations table for this file.
 *                 The fops table has functions like open, close, read, and write.
 *    inode -- The inode number of this file in the file system.
 *    fileposition -- The current position within the file that we are reading.
 *                    This will increment through the file as we read it.
 *    flags -- The only flag contained within this member is IN_USE or NOT_IN_USE.
 *             It is used to figure out which fds are available for use when trying
 *             to open a new file in a process.
 */
typedef struct file_descriptor_t {
	uint32_t * jumptable;
	int32_t inode;
	int32_t fileposition;
	int32_t flags;
} file_descriptor_t;

/* Explanation:
 * This is the PCB structure used by each process.  It is like a header that
 * contains all the relevant information that the OS might need to know as it
 * manipulates its processes.
 *    fds[8] -- The array of file descriptors, which represent each file that the
 *              process has open.
 *    filenames[8][32] -- An array holding the names of each of the open files
 *                        for quick access by the OS.
 *    parent_ksp -- The kernel stack pointer of the parent process.  This is used
 *                  to revert back to the parent kernel stack when we halt.
 *    parent_kbp -- The kernel base pointer of the parent process.  This is used
 *  				to revert back to the parent kernel stack when we halt.
 *    process_number -- The process number of this process.  It is a number from
 *                      1-7 (process 0 is the "no processes running" process).
 *    parent_process_number -- The process number of the parent process.  It is a
 *  						   number from 1-7 (process 0 is the "no processes running"
 *  						   process).
 *    argbuf -- Buffer for the arguments of this process.
 *    has_child -- Flag that indicates whether this process has a child process.
 *  			   If it has a child process, it will not be scheduled.
 *    tty_number -- The number of the tty in which this process is running.
 *    ksp_before_change -- This variable stores the KSP right before switching
 *  					   processes.
 *    kbp_before_change -- This variable stores the KBP right before switching
 *  					processes.
 */
typedef struct pcb_t {
	file_descriptor_t fds[8];
	uint8_t filenames[8][32]; 
	uint32_t parent_ksp;
	uint32_t parent_kbp;
	uint8_t process_number;
	uint8_t parent_process_number;
	uint8_t argbuf[100];
	uint32_t has_child;
	uint32_t tty_number;
	uint32_t ksp_before_change;
	uint32_t kbp_before_change;
} pcb_t;



/*** FUNCTION PROTOTYPES ***/

/*** System Calls ***/
/* Terminates a process, returning the specified value to its 
 * parent process. 
 */
int32_t halt(uint8_t status);

/* Attempts to load and execute a new program, handing off the 
 * processor to the new program until it terminates. 
 */
int32_t execute(const uint8_t* command);

/* Reads 'nbytes' bytes into 'buf' from the file corresponding to 
 * the given 'fd'. 
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* Writes 'nbytes' bytes from 'buf' into the file associated with 'fd'. 
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);

/* Attempts to open the file with the given filename and give it a spot
 * in the file array in the pcb associated with the current process. 
 */
int32_t open(const uint8_t* filename);

/* Closes the specifed file descriptor and makes it available for return from
 * later calls to open. 
 */
int32_t close(int32_t fd);

/* Reads the program’s command line arguments into a user-level buffer. */
int32_t getargs(uint8_t* buf, int32_t nbytes);

/* Maps the text-mode video memory into user space at a pre-set virtual address. */
int32_t vidmap(uint8_t** screen_start);

/* Related to signal handling. */
int32_t set_handler(int32_t signum, void* handler_address);

/* Related to signal handling. */
int32_t sigreturn(void);


/*** Other functions ***/ 
/* Called when we need to open stdin to initialize a new process. */
void open_stdin( int32_t fd );

/* Called when we need to open stdout to initialize a new process. */
void open_stdout( int32_t fd );

/*  Our test function for the execute syscall. */
void execute_test(void);

/* A function that literally does absolutely nothing. */
int32_t no_function(void);

/* Loads the initial three shells and jumps to the entry point of the first one. */
int32_t bootup(void);


/*** Set/Get functions ***/
/* Setter function */
void set_running_processes( uint8_t value );
/* Getter function */
uint8_t get_running_processes( void );
/* Setter function */
void set_kernel_stack_bottom( uint32_t value );
/* Getter function */
uint32_t get_kernel_stack_bottom( void );
/* Setter function */
void set_page_dir_addr( uint32_t value );
/* Getter function */
uint32_t get_page_dir_addr( void );
/* Setter function */
void set_current_process_number( uint8_t value );
/* Getter function */
uint8_t get_current_process_number( void );
/* Getter function */
uint32_t get_tty_number( void );



#endif /* SYSCALLS_H */
