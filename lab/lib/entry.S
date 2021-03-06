#include <inc/mmu.h>
#include <inc/memlayout.h>

.data
// Define the global symbols 'envs', 'pages', 'vpt', and 'vpd'
// so that they can be used in C as if they were ordinary global arrays.
	.globl envs
	.set envs, UENVS
	.globl pages
	.set pages, UPAGES
	.globl vpt
	.set vpt, UVPT
	#.set vpt, 0x10000000000
	.globl vpd
	.set   vpd, vpt+0x80000000
	.globl  vpde
	.set    vpde,vpd+0x400000
	.globl  vpml4e
	.set    vpml4e,vpde+0x2000


// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start
_start:
	// See if we were started with arguments on the stack
	movabs $USTACKTOP, %rax
	cmpq %rax,%rsp
	jne args_exist

	// If not, push dummy argc/argv arguments.
	// This happens when we are loaded by the kernel,
	// because the kernel does not know about passing arguments.
	pushq $0
	pushq $0

args_exist:
	movq 8(%rsp), %rsi
	movq (%rsp), %rdi
	call libmain
1:	jmp 1b

