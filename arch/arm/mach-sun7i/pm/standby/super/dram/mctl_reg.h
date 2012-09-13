#ifndef   _MCTL_REG_H
#define   _MCTL_REG_H

#ifndef MCTL_BASE
#define MCTL_BASE			0xF1c01000
#endif
#define MEM_BASE			0xC0000000
#define RTC_BASE			0xF1c20c00

#include "include.h"
#include "mctl_par.h"

#define SDR_CCR					(MCTL_BASE + 0x00)
#define SDR_DCR					(MCTL_BASE + 0x04)
#define SDR_IOCR				(MCTL_BASE + 0x08)
#define SDR_CSR					(MCTL_BASE + 0x0c)
#define SDR_DRR					(MCTL_BASE + 0x10)
#define SDR_TPR0				(MCTL_BASE + 0x14)
#define SDR_TPR1				(MCTL_BASE + 0x18)
#define SDR_TPR2				(MCTL_BASE + 0x1c)
#define SDR_RSLR0				(MCTL_BASE + 0x4c)
#define SDR_RSLR1				(MCTL_BASE + 0x50)
#define SDR_RDQSGR				(MCTL_BASE + 0x5c)
#define SDR_ODTCR				(MCTL_BASE + 0x98)
#define SDR_DTR0				(MCTL_BASE + 0x9c)
#define SDR_DTR1				(MCTL_BASE + 0xa0)
#define SDR_DTAR				(MCTL_BASE + 0xa4)
#define SDR_ZQCR0				(MCTL_BASE + 0xa8)
#define SDR_ZQCR1				(MCTL_BASE + 0xac)
#define SDR_ZQSR				(MCTL_BASE + 0xb0)
#define SDR_IDCR				(MCTL_BASE + 0xb4)
#define SDR_MR					(MCTL_BASE + 0x1f0)
#define SDR_EMR					(MCTL_BASE + 0x1f4)
#define SDR_EMR2				(MCTL_BASE + 0x1f8)
#define SDR_EMR3  			(MCTL_BASE + 0x1fc)


#define SDR_DLLCR				(MCTL_BASE + 0x200)
#define SDR_DLLCR0			(MCTL_BASE + 0x204)
#define SDR_DLLCR1			(MCTL_BASE + 0x208)
#define SDR_DLLCR2			(MCTL_BASE + 0x20c)
#define SDR_DLLCR3			(MCTL_BASE + 0x210)
#define SDR_DLLCR4			(MCTL_BASE + 0x214)
#define SDR_DQTR0				(MCTL_BASE + 0x218)
#define SDR_DQTR1				(MCTL_BASE + 0x21c)
#define SDR_DQTR2				(MCTL_BASE + 0x220)
#define SDR_DQTR3				(MCTL_BASE + 0x224)
#define SDR_DQSTR0			(MCTL_BASE + 0x228)
#define SDR_DQSTR1			(MCTL_BASE + 0x22c)
#define SDR_CR					(MCTL_BASE + 0x230)
#define SDR_DPCR					(MCTL_BASE + 0x23c)
#define SDR_APR  				(MCTL_BASE + 0x240)
#define SDR_LTR	  			(MCTL_BASE + 0x244)
#define SDR_HPCR				(MCTL_BASE + 0x250)
#define SDR_SCSR				(MCTL_BASE + 0x2e0)

#define SDR_GP_REG0				(RTC_BASE + 0x120)


#define mctl_read_w(n)   		(*((volatile uint32 *)(n)))
#define mctl_write_w(n,c) 	(*((volatile uint32 *)(n)) = (c))

#endif  //_MCTL_REG_H
