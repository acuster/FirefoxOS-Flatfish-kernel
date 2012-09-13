#include <mach/platform.h>
#include "../pm_config.h"


/**stack point address in sram*/
#define SP_IN_SRAM	0xf0007ffc

    .text
    .globl save_sp
save_sp:
    mov r0, r13
    ldr  r13, =SP_IN_SRAM
    mov pc,lr

    .text
    .globl restore_sp
restore_sp:
    mov r13, r0
    mov pc,lr

    .text
    .globl get_sp
get_sp:
    mov r0, r13
    mov pc,lr

	.text
	.globl standby_flush_tlb
standby_flush_tlb:
	push    {r0-r3}
	MOV     r0, #0
	/*instruction entire instruction tlb*/
	mcr p15, 0, r0, c8, c5, 0
	/* invalid entire data tlb */
	mcr p15, 0, r0, c8, c6, 0
	/*invalidate entire unified TLB inner shareable*/
	mcr p15, 0, r0, c8, c3, 0
	dsb
	ISB
	pop     {r0-r3}
	mov pc,lr

    .text
    .globl standby_preload_tlb
standby_preload_tlb:
	push    {r0-r3}
	/*16k*/
	ldr r0, =IO_ADDRESS(AW_SRAM_A1_BASE)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A1_BASE + 0x1000)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A1_BASE + 0x2000)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A1_BASE + 0x3000)
	ldr r1, [r0]

	/*16k*/
	ldr r0, =IO_ADDRESS(AW_SRAM_A2_BASE)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A2_BASE + 0x1000)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A2_BASE + 0x2000)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A2_BASE + 0x3000)
	ldr r1, [r0]

	/*16k*/
	ldr r0, =IO_ADDRESS(AW_SRAM_A3_A4_BASE)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A3_A4_BASE + 0x1000)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A3_A4_BASE + 0x2000)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(AW_SRAM_A3_A4_BASE + 0x3000)
	ldr r1, [r0]

	ldr r0, =IO_ADDRESS(SW_PA_PORTC_IO_BASE)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(SW_PA_UART0_IO_BASE)
	ldr r1, [r0]
	ldr r0, =IO_ADDRESS(SW_PA_TIMERC_IO_BASE)
	ldr r1, [r0]

	dsb
	isb

	pop     {r0-r3}
	mov pc,lr


    .text
    .globl disable_cache
disable_cache:
    push    {r0-r3}
    MRC     p15, 0, r0, c1, c0, 0       /*read cr                  */
    BIC	    r0, r0, #0x1 << 12
    BIC	    r0, r0, #0x1 << 2
    MCR     p15, 0, r0, c1, c0, 0       /*disable cache                  */
    ISB
    pop     {r0-r3}
    MOV     pc, lr

    .text
    .globl enable_cache
enable_cache:
    push    {r0-r3}
    MRC     p15, 0, r0, c1, c0, 0       /*read cr                  */
    ORR	    r0, r0, #0x1 << 12
    ORR	    r0, r0, #0x1 << 2
    MCR     p15, 0, r0, c1, c0, 0       /*disable cache                  */
    ISB
    pop     {r0-r3}
    MOV     pc, lr


    .text
    .globl flush_dcache
flush_dcache:
    push    {r0-r12}
    dmb					/* ensure ordering with previous memory accesses */
    MRC     p15, 1, r0, c0, c0, 1       /*read clidr                              */
    ANDS    r3, r0, #0x7000000          /*extract loc from clidr                  */
    MOV     r3, r3, lsr #23             /*left align loc bit field                */
    BEQ     finished                    /*if loc is 0, then no need to clean      */
    mov     r10, #0                     /*start clean at cache level 0            */
loop1:
    ADD     r2, r10, r10, lsr #1        /*work out 3x current cache level         */
    MOV     r1, r0, lsr r2              /*extract cache type bits from clidr      */
    AND     r1, r1, #7                  /*mask of the bits for current cache only */
    CMP     r1, #2                      /*see what cache we have at this level    */
    BLT     skip                        /*skip if no cache, or just i-cache       */
    MCR     p15, 2, r10, c0, c0, 0      /*select current cache level in cssr      */
    ISB                                 /*isb to sych the new cssr&csidr          */
    MRC     p15, 1, r1, c0, c0, 0       /*read the new csidr                      */
    AND     r2, r1, #7                  /*extract the length of the cache lines   */
    ADD     r2, r2, #4                  /*add 4 (line length offset)              */
    LDR     r4, =0x3ff
    ANDS    r4, r4, r1, lsr #3          /*find maximum number on the way size     */
    CLZ     r5, r4                      /*find bit position of way size increment */
    LDR     r7, =0x7fff
    ANDS    r7, r7, r1, lsr #13         /*extract max number of the index size    */
loop2:
    MOV     r9, r4                      /*create working copy of max way size     */
loop3:
    ORR     r11, r10, r9, lsl r5        /*factor way and cache number into r11    */
    ORR     r11, r11, r7, lsl r2        /*factor index number into r11            */
    MCR     p15, 0, r11, c7, c14, 2	      /*clean & invalidate by set/way                  */
    SUBS    r9, r9, #1                  /*decrement the way                       */
    BGE     loop3                       /*                                        */
    SUBS    r7, r7, #1                  /*decrement the index                     */
    BGE     loop2                       /*                                        */
skip:                                   /*                                        */
    ADD     r10, r10, #2                /*increment cache number                  */
    CMP     r3, r10                     /*                                        */
    BGT     loop1                       /*                                        */
finished:                                /*                                        */
    MOV     r10, #0                     /*swith back to cache level 0             */

    MCR     p15, 2, r10, c0, c0, 0      /*select current cache level in cssr      */
    dsb
    ISB                                 /*                                        */
    pop    {r0-r12}
    MOV     pc, lr                      /*                                        */

    .text
    .globl flush_icache
flush_icache:
    push    {r0-r3}
    MOV     r0, #0
    MCR     p15, 0, r0, c7, c5, 0       /*I+BTB cache invalidate                  */
    dsb
    ISB
    pop     {r0-r3}
    MOV     pc, lr

	.text
	.globl invalidate_icache
invalidate_icache:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c1, 0		@ invalidate I-cache inner shareable
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
	dsb
	ISB
	mov	pc, lr