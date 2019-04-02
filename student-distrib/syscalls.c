/*************************************************/
/* syscalls.c - The system call implementations. */
/*************************************************/

#include "syscalls.h"
#include "interrupthandler.h"
#include "keyboard.h"
#include "rtc.h"
#include "files.h"


/*** GLOBAL VARIABLES ***/
/* The bitmask representing running processes. */
uint8_t running_processes = 0x80;

/* The address of the current process's kernel stack bottom. */
uint32_t kernel_stack_bottom;

/* Address of the page directory for the current process. */
uint32_t page_dir_addr;

/* Number of the current running process, defined from 0-7 */
uint8_t current_process_number = 0;



/*
 * Initialize the file operations tables -- we will make the
 * file descriptors' jumptable pointers point to these tables when
 * we open a file.
 */
 
/* stdin file operations table */
uint32_t stdin_fops_table[4] = { (uint32_t)(no_function),
								 (uint32_t)(terminal_read),
								 (uint32_t)(no_function),
								 (uint32_t)(no_function) };
/* stdout file operations table */
uint32_t stdout_fops_table[4] = { (uint32_t)(no_function),
								  (uint32_t)(no_function),
								  (uint32_t)(terminal_write),
								  (uint32_t)(no_function) };
/* rtc file operations table */
uint32_t rtc_fops_table[4] = { (uint32_t)(rtc_open),
							   (uint32_t)(rtc_read),
							   (uint32_t)(rtc_write),
							   (uint32_t)(rtc_close) };
/* file file operations table */
uint32_t file_fops_table[4] = { (uint32_t)(file_open),
							    (uint32_t)(file_read),
							    (uint32_t)(file_write),
							    (uint32_t)(file_close) };
/* directory file operations table */
uint32_t dir_fops_table[4] = { (uint32_t)(dir_open),
							   (uint32_t)(dir_read),
							   (uint32_t)(dir_write),
							   (uint32_t)(dir_close) };
							   
			   
/*
 * halt()
 *
 * Terminates a process, returning the specified value to its parent process by switching
 * back to the parent's kernel stack and then "finishing" the execute that called this process.
 *
 * Inputs: status of the terminated process
 * Retvals: status of the terminated process (same as the input)
 * 
 */
int32_t halt(uint8_t status)
{
	/* Local variables */
	int i;
	
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	
	/* Prevent the user from closing the final shell
	 * NOTE -- In order to do this, we just restart the shell
	 */
	if( process_control_block->parent_process_number == 0 )
	{
		printf("Silly rabbit, trix are for kids.\n");
		
		/* Get the entry point to shell */
		uint8_t buf[4];
		if( -1 == fs_read((const int8_t *)("shell"), ENTRY_POINT_OFFSET, buf, 4) )
		{
			return -1;
		}
		
		/* Save the entry point */
		uint32_t entry_point;
		for( i = 0; i < 4; i++ )
		{
			entry_point |= (buf[i] << 8*i);
		}
		
		/* Jump back to the start of the shell */
		to_the_user_space(entry_point);
	}
	
	/* 
	 * Mark the current process as 0, aka this process is done and its slot
	 * is now available for new processes.
	 */
	uint8_t bitmask = 0x7F;
	for( i = 0; i < process_control_block->process_number; i++ )
	{
		bitmask = (bitmask >> 1) + 0x80;
	}
	running_processes &= bitmask;
	
	/* Set the current process number back to the parent */
	current_process_number = process_control_block->parent_process_number;
	
	/* Reset the parent to have no children */
	pcb_t * parent_pcb = (pcb_t *)( _8MB - (_8KB)*(process_control_block->parent_process_number + 1) );
	parent_pcb->has_child = 0;
	
	/* Load the page directory of the parent. */
	page_dir_addr = (uint32_t)(&page_directories[process_control_block->parent_process_number]);
	asm (
	"movl page_dir_addr, %%eax        ;"
	"andl $0xFFFFFFE7, %%eax          ;"
	"movl %%eax, %%cr3                ;"
	"movl %%cr4, %%eax                ;"
	"orl $0x00000090, %%eax           ;"
	"movl %%eax, %%cr4                ;"
	"movl %%cr0, %%eax                ;"
	"orl $0x80000000, %%eax 	      ;"
	"movl %%eax, %%cr0                 "
	: : : "eax", "cc" );
	
	/* Set the kernel_stack_bottom and the TSS to point back at the parent's kernel stack. */
	kernel_stack_bottom = tss.esp0 = _8MB - (_8KB)*process_control_block->parent_process_number - 4;
	
	/* 
	 * Switch the kernel stack back to the parent's kernel stack by
	 * restoring ESP and EBP.
	 * -- NOTE: We need to store "status" since we will be losing it when we
	 *          switch the stack. To do this, move ESP to the new stack,
	 *          push status (which is referenced by EBP), move EBP to the new
	 *          stack, then pop status back into EAX for the return of halt.
	 */
	uint32_t the_status = status;
	
	/* Put the "parent_ksp" into the %ESP. */
	asm volatile("movl %0, %%esp	;"
				 "pushl %1			;"
				 ::"g"(process_control_block->parent_ksp),"g"(the_status));
				 
	/* Put the "parent_kbp" into the %EBP. */
	asm volatile("movl %0, %%ebp"::"g"(process_control_block->parent_kbp));
	
	asm volatile("popl %eax");
	
	/* 
	 * We have now switched back to the stack of the parent. Remember that the parent
	 * stack was where we originally called 'execute' for this process. Thus, the parent's
	 * stack (now the current stack) contains the appropriate data that was pushed when 
	 * we made the 'execute' syscall. If we now leave and ret, we will return back to the
	 * syscall_handler for the original 'execute' syscall, which can then iret.
	 * -- NOTE: We never iret from the 'halt' command... we just ditch its stack information
	 *          when we switch back to the parent stack.
	 */
	asm volatile("leave");
	asm volatile("ret");
	
	return 0;
}

/*
 * execute()
 *
 * Attempts to load and execute a new program, handing off the processor to the
 * new program until it terminates.
 *
 * Inputs: command string
 * Retvals:
 * -1: command cannot be executed
 * 256: program dies by an exception
 * 0 to 255: program executes a halt system call, in which case the value returned 
 * 			 is that given by the program’s call to halt
 */
int32_t execute(const uint8_t* command)
{
	/* Local variables. */
	uint8_t fname[32];
	uint8_t buf[4];
	uint32_t i;
	uint32_t entry_point;
	uint8_t magic_nums[4] = {0x7f, 0x45, 0x4c, 0x46};
	uint8_t open_process;
	uint32_t first_space_reached;
	uint32_t length_of_fname;
	uint8_t localargbuf[TERMINAL_BUFFER_MAX_SIZE];
	
	/* Initializations. */
	entry_point = 0;
	first_space_reached = 0;
	length_of_fname = 0;
	
	/* Check for an invalid command. */
	if( command == NULL )
	{
		return -1;
	}
	
	/* 
	 * Get the file name of the program to be executed and store
	 * the additional args into the argbuf.
	 */
	for( i = 0; command[i] != '\0' ; i++ )
	{
		if( command[i] == ' ' && first_space_reached == 0 )
		{
			first_space_reached = 1;
			length_of_fname = i;
			fname[i] = '\0';
		}
		else if( first_space_reached == 1 )
		{
			localargbuf[i-length_of_fname-1] = command[i];
		}
		else
		{
			if(i>=32 && first_space_reached == 0){
				return -1;
			}
			fname[i] = command[i];
		}
	}
	localargbuf[i-length_of_fname-1] = '\0';
	
	if( first_space_reached == 0 )
	{
		fname[i] = '\0';
	}
	
	/* 
	 * Read the identifying 4 bytes from the file into buf which
	 * identify an executable program image or not. 
	 */
	if( -1 == fs_read((const int8_t *)fname, 0, buf, 4) )
	{
		return -1;
	}
	
	/* Ensure an executable program image. */
	if( 0 != strncmp((const int8_t*)buf, (const int8_t*)magic_nums, 4) )
	{
		return -1;
	}
	
	/* Look for an open slot for the process. */
	uint8_t bitmask = 0x80;
	for( i = 0; i < 8; i++ ) 
	{
		if( !(running_processes & bitmask) ) 
		{
			open_process = i;
			running_processes |= bitmask;
			current_process_number = open_process;
			break;
		}
		bitmask >>= 1;
		if( bitmask == 0 )
		{
			return -1;
		}
	}
	
	/* Get the entry point to the program. */
	if( -1 == fs_read((const int8_t *)fname, ENTRY_POINT_OFFSET, buf, 4) )
	{
		return -1;
	}
	
	/* Save the entry point. */
	for( i = 0; i < 4; i++ )
	{
		entry_point |= (buf[i] << 8*i);
	}
	
	/* Set up the new page directory for the new task. */
	if( -1 == setup_new_task( open_process ) )
	{
		return -1;
	}
	
	/* Load the program to the appropriate starting address. */
	fs_load((const int8_t *)fname, PROGRAM_LOAD_ADDR);
	
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)( _8MB - (_8KB)*(open_process + 1) );
	
	/* Store the %ESP as "parent_ksp" in the PCB. */
	uint32_t esp;
	asm volatile("movl %%esp, %0":"=g"(esp));
	process_control_block->parent_ksp = esp;
	
	/* Store the %EBP as "parent_kbp" in the PCB. */
	uint32_t ebp;
	asm volatile("movl %%ebp, %0":"=g"(ebp));
	process_control_block->parent_kbp = ebp;
	
	if( running_processes == 0xC0 )
	{
		/* 
		 * If this process was the first process called (aka, it was called from 
		 * "no processes running" process), make the parent process number 0. 
		 * -- NOTE: we must do it like this because the "no processes running" 
		 * process does not have a PCB.
		 */
		process_control_block->parent_process_number = 0;
		
		/* If this is the first process, initialize it to tty #1. */
		process_control_block->tty_number = 1;
	}
	else
	{
		/* 
		 * Set the parent_process_number of the new process to be the process 
		 * number of the current process that called it (find this in the PCB).
		 */
		process_control_block->parent_process_number = ( (pcb_t *)(esp & ALIGN_8KB) )->process_number;
		
		/* Indicate that the parent process has a child */
		( (pcb_t *)(esp & ALIGN_8KB) )->has_child = 1;
		
		/* Set the tty number of this process to be the same as the parent */
		process_control_block->tty_number = ( (pcb_t *)(esp & ALIGN_8KB) )->tty_number;
	}
	process_control_block->process_number = open_process;
	
	/* Initialize fields in the PCB for each file descriptor. */
	for( i = 0; i < 8; i++ )
	{
		process_control_block->fds[i].inode = 0;
		process_control_block->fds[i].fileposition = 0;
		process_control_block->fds[i].flags = NOT_IN_USE;
	}
	
	/* Store the args passed to this function into the PCB. */
	strcpy((int8_t*)process_control_block->argbuf, (const int8_t*)localargbuf);
	
	/* Set the kernel_stack_bottom and tss.esp0 field to be the bottom of the new kernel stack. */
	kernel_stack_bottom = tss.esp0 = _8MB - (_8KB)*open_process - 4;
	
	/* Call open for stdin and stdout. */
	open( (uint8_t *) "stdin"  );
	open( (uint8_t *) "stdout" );
	
	/* Jump to the entry point and begin execution. */
	to_the_user_space(entry_point);

	return 0;
}

/*
 * execute_test()
 *
 * Our test function for the execute syscall.
 *
 * Inputs: none
 * Retvals: none
 */
void execute_test(void)
{
	const uint8_t * test_string = (uint8_t *) "shell";
	execute(test_string);
}

/*
 * bootup()
 *
 * Loads the initial three shells and jumps to the entry point of the first one.
 *
 * Inputs: none
 * Retvals: 0
 * 
 */
int32_t bootup(void)
{
	/* Local variables. */
	uint8_t buf[4];
	uint32_t i;
	uint32_t j;
	uint32_t entry_point;
	uint32_t esp;
	uint32_t ebp;
	pcb_t * process_control_block;
	
	/* Initializations. */
	entry_point = 0;
	
	/* Get the entry point to shell. */
	if( -1 == fs_read((const int8_t *)("shell"), ENTRY_POINT_OFFSET, buf, 4) )
	{
		return -1;
	}
	
	/* Save the entry point. */
	for( i = 0; i < 4; i++ )
	{
		entry_point |= (buf[i] << 8*i);
	}
	
	/* Setup each shell. */
	for( i = 3; i > 0; i-- )
	{
		/* Set up the new page directory for the new shell. */
		if( -1 == setup_new_task(i) )
		{
			return -1;
		}
		
		/* Load the program to the appropriate starting address. */
		fs_load((const int8_t *)("shell"), PROGRAM_LOAD_ADDR);
		
		/* Get a pointer to the PCB. */
		process_control_block = (pcb_t *)( _8MB - (_8KB)*(i + 1) );
	
		/* Store the %ESP as "parent_ksp" in the PCB. */
		asm volatile("movl %%esp, %0":"=g"(esp));
		process_control_block->parent_ksp = esp;
	
		/* Store the %EBP as "parent_kbp" in the PCB. */
		asm volatile("movl %%ebp, %0":"=g"(ebp));
		process_control_block->parent_kbp = ebp;
		
		/* Set the parent process number to be 0. */
		process_control_block->parent_process_number = 0;
		
		/* Set the process number. */
		process_control_block->process_number = i;
		
		/* Initialize fields in the PCB for each file descriptor. */
		for( j = 0; j < 8; j++ )
		{
			process_control_block->fds[j].inode = 0;
			process_control_block->fds[j].fileposition = 0;
			process_control_block->fds[j].flags = NOT_IN_USE;
		}
		
		/* The shells initially have no children. */
		process_control_block->has_child = 0;
		
		/* Set the shell's terminal number. */
		process_control_block->tty_number = i-1;
		
		/* 
		 * Set the kernel_stack_bottom and tss.esp0 field to be the bottom 
		 * of the new kernel stack.
		 */
		kernel_stack_bottom = tss.esp0 = _8MB - (_8KB)*i - 4;
		
		if( i != 1 )
		{
			/* Push things onto the kernel stack to initialize task switching. */
			asm volatile("movl %%esp, %%eax      ;"
						 "movl %%ebp, %%ebx      ;"
						 "movl %0, %%ecx		 ;"
						 "movl %3, %%edx		 ;"
						 "movl %%ecx, %%esp      ;"
						 "movl %%ecx, %%ebp      ;"
						 "pushl %1               ;"
						 "pushl $0x83FFFF0       ;"
						 "pushl $0               ;"
						 "pushl %2               ;"
						 "pushl %%edx            ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $0               ;"
						 "pushl $end_pit_handler ;"
						 "pushl %%ecx            ;"
						 "movl %%eax, %%esp      ;"
						 "movl %%ebx, %%ebp      ;"
						 :: "g"(kernel_stack_bottom), "g"(USER_DS), "g"(USER_CS), 
							"g"(entry_point): "eax", "ebx", "ecx", "edx");
		}
		
		/* Store KSP and KBP before change. */
		process_control_block->ksp_before_change = 
			kernel_stack_bottom - INITIAL_KERNEL_STACK_SIZE;
		process_control_block->kbp_before_change = 
			kernel_stack_bottom - INITIAL_KERNEL_STACK_SIZE;

	
		/* Call open for stdin and stdout. */
		open( (uint8_t*) "stdin" );
		open( (uint8_t*) "stdout");
	}
	
	/* Initialize the video memory buffers */
	memcpy((char *)VIDEO_BUF1, (char *)VIDEO, _4KB);
	memcpy((char *)VIDEO_BUF2, (char *)VIDEO, _4KB);
	memcpy((char *)VIDEO_BUF3, (char *)VIDEO, _4KB);
	
	
	/* Update the running processes bitmask. */
	running_processes |= INITIAL_SHELLS_BITMASK;
	
	current_process_number = 1;
	
	/* Enable interrupts */
	sti();
	
	/* Jump to the entry point and begin execution. */
	to_the_user_space(entry_point);

	return 0;
}

/*
 * read()
 *
 * Reads 'nbytes' bytes into 'buf' from the file corresponding to the
 * given 'fd'.
 *
 * Inputs:
 * fd: file descriptor
 * buf: buffer to read from
 * nbytes: number of bytes to read from
 *
 * Retvals: number of bytes read
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes)
{
	/* Restore interrupts. */
	sti();

	/* Local variables. */
	int bytesread;
	
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	/* Check for invalid fd or buf. */
	if( fd < 0 || fd > 7 || buf == NULL || process_control_block->fds[fd].flags == NOT_IN_USE )
	{
			return -1;
	}

	/* Get the filename and fileposition of the file with the given fd. */
	uint8_t* filename = process_control_block->filenames[fd];
	uint32_t fileposition = process_control_block->fds[fd].fileposition;

	/* Push arguments for the file's read function and call it. */
	asm volatile("pushl %0          ;"
							 "pushl %1              ;"
							 "pushl %2              ;"
							 "pushl %3              ;"
							 "call  *%4             ;"
							 :
							 : "g" (fileposition), "g" ((int32_t)filename), "g" (nbytes), "g" ((int32_t)buf),
							   "g" (process_control_block->fds[fd].jumptable[1]));
							 
	/* Store the return value from the read function. */
	asm volatile("movl %%eax, %0":"=g"(bytesread));
	asm volatile("addl $16, %esp    ;");
	
	/* Update the current file's fileposition. */
	process_control_block->fds[fd].fileposition += bytesread;
	
	return bytesread;
}

/*
 * write()
 *
 * Writes 'nbytes' bytes from 'buf' into the file associated with 'fd'.
 *
 * Inputs: fd - file descriptor to which we want to write
 *         buf - the buffer containing the data we want to write
 *         nbytes - the number of bytes we want to write
 * Retvals:
 * -1: invalid fd or buf
 * n: number of bytes written to the buffer
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes)
{	
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	/* Check for invalid fd or buf. */
	if( fd < 0 || fd > 7 || buf == NULL || process_control_block->fds[fd].flags == NOT_IN_USE )
	{
			return -1;
	}

	/* Push arguments for the file's write function and call it. */
	asm volatile("pushl %0          ;"
				 "pushl %1              ;"
				 "call  *%2             ;"
				 "leave					;"
				 "ret					;"
				 :
				 : "g" (nbytes), "g" ((int32_t)buf), "g" (process_control_block->fds[fd].jumptable[2]));
							 
	return 0;
}

/*
 * open()
 *
 * Attempts to open the file with the given filename and give it a spot
 * in the file array in the pcb associated with the current process.
 *
 * Inputs: the filename (string)
 * Retvals:
 * -1: failure
 * 0: success
 * 
 */
int32_t open(const uint8_t* filename)
{
	/* Local variables. */
	int i;
	dentry_t tempdentry;

	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);

	/* Call appropriate function for opening stdin. */
	if( 0 == strncmp((const int8_t*)filename, (const int8_t*)"stdin", 5) ) 
	{
		open_stdin( 0 );
		return 0;
	}
	
	/* Call appropriate function for opening stdout. */
	if( 0 == strncmp((const int8_t*)filename, (const int8_t*)"stdout", 5) ) 
	{
		open_stdout( 1 );
		return 0;
	}
	
	/* Get dentry information associated with the filename. */
	if( -1 == read_dentry_by_name(filename, &tempdentry)) {
		return -1;
	}

	/* 
	 * Look for the next open slot in the file array and populate 
	 * file descriptor information for this file in the PCB. 
	 */
	for (i=2; i<8; i++) 
	{
		if (process_control_block->fds[i].flags == NOT_IN_USE) 
		{	
			/* RTC */
			if (tempdentry.filetype == FILE_TYPE_RTC)
			{ 		
				if (-1 == rtc_open()) {
					return -1;
				} else {
					process_control_block->fds[i].jumptable = rtc_fops_table;
				}
			}
			
			/* Directory */
			else if(tempdentry.filetype == FILE_TYPE_DIRECTORY)
			{ 
				process_control_block->fds[i].jumptable = dir_fops_table;
			}
			
			/* Regular File */
			else if(tempdentry.filetype == FILE_TYPE_REGULAR_FILE)
			{ 
				process_control_block->fds[i].jumptable = file_fops_table;
			}

			/* Fill in remaining data and return the new fd. */
			process_control_block->fds[i].flags = IN_USE;
			process_control_block->fds[i].inode = tempdentry.inode;
			strcpy((int8_t*)process_control_block->filenames[i], (const int8_t*)filename);
			return i;
		}		
	}

	/* Print an error message if the file array is full. */
	printf("The File Descriptor Array is Filled\n");
	return -1;	
}

/*
 * open_stdin()
 *
 * Called when we need to open stdin to initialize a new process.
 *
 * Inputs: the file descriptor we want to use
 * Retvals: none
 */
void open_stdin( int32_t fd )
{
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	/* Set the jumptable -- NOTE: for stdin, we only have a read function. */
	process_control_block->fds[fd].jumptable = stdin_fops_table;
	
	/* Mark this fd as in use. */
	process_control_block->fds[fd].flags = IN_USE;
}

/*
 * open_stdout()
 *
 * Called when we need to open stdout to initialize a new process.
 *
 * Inputs: the file descriptor we want to use
 * Retvals: none
 */
void open_stdout( int32_t fd )
{
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	/* Set the jumptable -- NOTE: for stdout, we only have a write function. */
	process_control_block->fds[fd].jumptable = stdout_fops_table;
	
	/* Mark this fd as in use. */
	process_control_block->fds[fd].flags = IN_USE;
}

/*
 * close()
 *
 * Closes the speciﬁed ﬁle descriptor and makes it available for return from
 * later calls to open.
 *
 * Inputs: the file descriptor we want to close
 * Retvals:
 * -1: error
 * 0: success
 */
int32_t close(int32_t fd)
{
	/* Local variables. */
	uint32_t retval;
	
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	/* Check for an invalid fd. */
	if( fd < 2 || fd > 7 || process_control_block->fds[fd].flags == NOT_IN_USE )
	{
		return -1;
	}
	
	/* Call the file's close function. */
	asm volatile("call  *%0		;"
				 :
				 : "g" (process_control_block->fds[fd].jumptable[3]));
				 
	/* Store the return value from the close function. */
	asm volatile("movl %%eax, %0":"=g"(retval));
	
	/* Reset file descriptor information associated with the file. */
	process_control_block->fds[fd].jumptable = NULL;
	process_control_block->fds[fd].inode = 0;
	process_control_block->fds[fd].fileposition = 0;
	process_control_block->fds[fd].flags = NOT_IN_USE;
	
	return retval;
}

/*
 * getargs()
 *
 * Reads the program’s command line arguments into a user-level buffer.
 *
 * Inputs: buf - pointer to a user-level buffer
 *         nbytes - number of bytes to read from the agruments buffer
 * Retvals
 * -1: arguments and a terminal null byte do not fit into buffer
 * 0: success
 */
int32_t getargs(uint8_t* buf, int32_t nbytes)
{
	/* Check for invalid parameters. */
	if( buf == NULL || nbytes == 0 )
	{
		return -1;
	}
	
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	/* Ensure we are not asking for less characters than the argbuf (?) */
	if( strlen((const int8_t*)process_control_block->argbuf) > nbytes )
	{
		return -1;
	}
	
	/* Copy the args to the user-level buffer. */
	strcpy((int8_t*)buf, (const int8_t*)process_control_block->argbuf);
	
	return 0;
}

/*
 * vidmap()
 *
 * Maps the text-mode video memory into user space at a pre-set virtual address.
 *
 * Inputs: the address of a pointer in user-space that needs to get the
 *         pointer to video memory
 * Retvals:
 * 0: success
 * -1: the (input) address of the pointer is not in user-space, aka invalid
 */
int32_t vidmap(uint8_t** screen_start)
{
	/* Ensure screen_start is within proper bounds. */
	if( (uint32_t) screen_start < _128MB || (uint32_t) screen_start > (_128MB + _4MB) )
	{
		return -1;
	}

	switch( get_tty_number() )
	{
		case 0:
			*screen_start = (uint8_t *) VIDEO_BUF1;
			break;
		case 1:
			*screen_start = (uint8_t *) VIDEO_BUF2;
			break;
		case 2:
			*screen_start = (uint8_t *) VIDEO_BUF3;
			break;
	}
	
	return 0;
}

/*
 * set_handler()
 *
 * Related to signal handling.
 *
 * Inputs: signum - number of the signal
 *         handler_address - pointer to the handler function
 * Retvals
 * 0: always
 */
int32_t set_handler(int32_t signum, void* handler_address)
{
	return 0;
}

/*
 * sigreturn()
 *
 * Related to signal handling.
 *
 * Inputs: none
 * Retvals:
 * 0: always
 */
int32_t sigreturn(void)
{
	return 0;
}

/*
 * no_function()
 *
 * A function that literally does absolutely nothing. 
 *
 * Inputs: none
 * Retvals:
 * 0: always
 */
int32_t no_function(void)
{
	return 0;
}

/*
 * set_running_processes
 *
 * Set the running_processes variable from an external location. 
 *
 * Inputs: setter value.
 *
 * Retvals: none
 */
void set_running_processes( uint8_t value )
{
	running_processes = value;
}

/*
 * get_running_processes
 *
 * Get the running_processes variable for an external location. 
 *
 * Inputs: none.
 *
 * Retvals: getter value.
 */
uint8_t get_running_processes( void )
{
	return running_processes;
}

/*
 * set_kernel_stack_bottom
 *
 * Set the running_processes variable from an external location. 
 *
 * Inputs: setter value.
 *
 * Retvals: none
 */
void set_kernel_stack_bottom( uint32_t value )
{
	kernel_stack_bottom = value;
}

/*
 * get_kernel_stack_bottom
 *
 * Gets kernel_stack_bottom.
 *
 * Inputs: none.
 *
 * Retvals: getter value.
 */
uint32_t get_kernel_stack_bottom( void )
{
	return kernel_stack_bottom;
}

/*
 * set_page_dir_addr
 *
 * Sets page_dir_addr
 *
 * Inputs: setter value.
 *
 * Retvals: none
 */
void set_page_dir_addr( uint32_t value )
{
	page_dir_addr = value;
}

/*
 * get_page_dir_addr
 *
 * Gets page_dir_addr.
 *
 * Inputs: none.
 *
 * Retvals: getter value.
 */
uint32_t get_page_dir_addr( void )
{
	return page_dir_addr;
}

/*
 * set_current_process_number
 *
 * Sets current_process_number
 *
 * Inputs: setter value.
 *
 * Retvals: none
 */
void set_current_process_number( uint8_t value )
{
	current_process_number = value;
}

/*
 * get_current_process_number
 *
 * Gets current_process_number.
 *
 * Inputs: none.
 *
 * Retvals: getter value.
 */
uint8_t get_current_process_number( void )
{
	return current_process_number;
}

/*
 * get_tty_number
 *
 * Gets tty_number of the current process.
 *
 * Inputs: none.
 *
 * Retvals: getter value.
 */
uint32_t get_tty_number( void )
{
	/* Extract the PCB from the KBP */
	pcb_t * process_control_block = (pcb_t *)(kernel_stack_bottom & ALIGN_8KB);
	
	return process_control_block->tty_number;
}
