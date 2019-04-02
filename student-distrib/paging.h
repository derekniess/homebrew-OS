/**********************************/
/* paging.h - The paging manager. */
/**********************************/
#ifndef PAGING_H
#define PAGING_H



#include "x86_desc.h"
#include "types.h"



#define	TABLE_ADDRESS_SHIFT		12
#define MAX_NUM_OF_PROCESSES	8
#define PROGRAM_IMG_ENTRY		0x20



/* Called from kernel.c to initialize paging. */
int32_t init_paging(void);

/* Called from 'execute' to set up a new page directory. */
int32_t setup_new_task( uint8_t process_number );

#endif /* PAGING_H */

