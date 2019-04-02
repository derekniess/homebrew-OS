#######################################################################
## interrupthandler.h -  Set up interrupt handler assembly wrappers. ##
#######################################################################

#define ASM     1
#include "x86_desc.h"

.global syscall_handler
.global test_syscall
.global to_the_user_space


# INTERRUPT HANDLER MACRO
# The interrupt wrapper for a keyboard/rtc/pit generated interrupt.
# Interrupt handler must have use an assembly wrapper because it's an interrupt!
# The most important instruction that this code contains is the iret command
# This iret command returns the instruction pointer back to the interrupted program
# This couldn't be done in C code as inline assembly because the iret line would have to 
# come before the C functions leave and ret command, thereby rendering it useless
# Inputs   : none
# Outputs  : none
# Registers: saves and restores ebp, eax, ebx, ecx, edx, edi, esi, fl
#define HANDLER(name,end_name,send_to_fn)	\
.GLOBL name									;\
.GLOBL end_name								;\
name:										;\
	pushl %ebp								;\
	pushl %eax								;\
	pushl %ebx								;\
	pushl %ecx								;\
	pushl %edx								;\
	pushl %edi								;\
	pushl %esi								;\
	pushfl									;\
	call send_to_fn							;\
end_name:									;\
	popfl									;\
	popl %esi								;\
	popl %edi								;\
	popl %edx								;\
	popl %ecx								;\
	popl %ebx								;\
	popl %eax								;\
	popl %ebp								;\
	sti										;\
	iret

# keyboard_handler: interrupt handler for keyboard interrupts
HANDLER(keyboard_handler, end_keyboard_handler, keyboard_interruption);
# clock_handler: interrupt handler for rtc interrupts
HANDLER(clock_handler, end_clock_handler, clock_interruption);
# pit_handler: interrupt handler for pit interrupts
HANDLER(pit_handler, end_pit_handler, pit_interruption);



# A jump table to C functions that implement the system calls themselves.
syscall_jumptable:
	.long 0x0
	.long halt
	.long execute
	.long read
	.long write
	.long open
	.long close
	.long getargs
	.long vidmap
	.long set_handler
	.long sigreturn

# syscall_handler()
# Saves registers and jumps to respective C-implemented system call function.
# Inputs   : none
# Outputs  : none
# Registers: edx, ecx, ebx, eax act as inputs
syscall_handler:
#	pushl %xes
#	pushl %xds
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx

	pushl %edx 			# Argument 3
	pushl %ecx			# Argument 2
	pushl %ebx			# Argument 1

	cmpl $1, %eax		# Check that eax is greater than 1
	jl bad_eax
	cmpl $10, %eax		# Check that eax is less than 10
	jg bad_eax
	
	movl syscall_jumptable(,%eax,4),%eax
	call *%eax
	jmp end_syscall
	
bad_eax:
	movl $-1, %eax

end_syscall:
	addl $12,%esp		# Pop the args
	
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
#	popl %xds
#	popl %xes

	iret



# to_the_user_space()
# Description: In order to jump back to the user space we must return from
# the user level syscall interruption and rebuild the stack as shown below
# in  order to fake the return from interruption
#----TOP OF STACK----#
		#EIP
		#USER_CS
		#EFLAGS
		#USER_DS
# Inputs   :none
# Outputs  :none
# Registers: %EBX, %EAX
to_the_user_space:

	cli
	
	movl 4(%esp),%ebx 		# Extended Instruction Pointer -> ebx

	movl $USER_DS, %eax 	# Load segement selectors
	movw %ax, %gs 
	movw %ax, %fs  
	movw %ax, %ds 
	movw %ax, %es

	pushl %eax              # push the USER_DS to the top of the stack

	pushl $0x83FFFF0 		# push the User Space Extended Stack Pointer to the top of the stack

	pushf 					# push the flags to the stack
	popl %eax 				# pull them back down to...
	orl $0x4200, %eax 		# be altered appropriately
	pushl %eax 				# push the altered flags back onto the stack
	pushl $USER_CS 			# push the USER_CS to the top of the stack
	pushl %ebx 				# push the Extended Instruction Pointer to the top of the stack

	iret					#Return from interruption
