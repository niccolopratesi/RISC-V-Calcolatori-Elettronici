.section .init
.global _entry
_entry:
        # This is needed in order to produce a working executable
        # alongside the changes made in the linker file kernel.ld
        .option push
        .option norelax
        la gp, __global_pointer$
	# set up a stack for C.
        # stack0 is declared in boot_main.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0
        li a0, 1024*4
	csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0
        # Since we are still in a C environment, we should
        # call the ctors function before switching to sistema,
        # which uses a C++.
        call ctors
	# jump to start in start.s
        call start
