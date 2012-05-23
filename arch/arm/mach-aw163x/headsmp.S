#include <linux/linkage.h>
#include <linux/init.h>

        .section ".text.head", "ax"
	__CPUINIT

ENTRY(aw163x_secondary_startup)
	msr	cpsr_fsxc, #0xd3
	mov r0, #0
	ldr r1, =0xf34
        b       secondary_startup
ENDPROC(aw163x_secondary_startup)
