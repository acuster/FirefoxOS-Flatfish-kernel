#ifndef   MCTL_PAR_H
#define   MCTL_PAR_H

#include "include.h"

//DDR2_32B				DDR2_32B_64Mx16x2
//DDR2_16B				DDR2_16B_64Mx16x1
//DDR3_32B				DDR3_32B_128Mx16x2
//DDR3_16B				DDR3_32B_128Mx16x1
//DDR3_32B8				DDR3_32B_256Mx8x4

//*****************************************************************************
//DDR2 SDRAM(x16)
//*****************************************************************************

#ifdef DDR2_FPGA_BJ530
//DDR2 64Mx16 (128M Byte)
#define MCTL_DDR_TYPE			2				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			16
#define MCTL_CHIP_SIZE			1024			//unit in Mb
#define MCTL_CAS				5
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			13
#define MCTL_BUS_WIDTH			32
#endif

#ifdef DDR2_FPGA_S2C
//DDR2 64Mx16 (128M Byte)
#define MCTL_DDR_TYPE			2				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			8
#define MCTL_CHIP_SIZE			1024			//unit in Mb
#define MCTL_CAS				5
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			14
#define MCTL_BUS_WIDTH			32
#endif

#ifdef DDR2_16B
//DDR2 64Mx16 (128M Byte)
#define MCTL_DDR_TYPE			2				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			16
#define MCTL_CHIP_SIZE			1024			//unit in Mb
#define MCTL_CAS				5
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			13
#define MCTL_BUS_WIDTH			16
#endif


#ifdef DDR2_32B
//DDR2 64Mx16 (128M Byte)
#define MCTL_DDR_TYPE			2				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			16
#define MCTL_CHIP_SIZE			1024			//unit in Mb
#define MCTL_CAS				5
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			13
#define MCTL_BUS_WIDTH			32
#endif

#ifdef DDR2_16B
//DDR2 64Mx16 (128M Byte)
#define MCTL_DDR_TYPE			2				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			16
#define MCTL_CHIP_SIZE			1024			//unit in Mb
#define MCTL_CAS				5
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			13
#define MCTL_BUS_WIDTH			16
#endif


//*****************************************************************************
//DDR3 SDRAM(x16)
//*****************************************************************************

#ifdef DDR3_32B8
//DDR3 256Mx8 (256M Byte)
#define MCTL_DDR_TYPE			3				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			8
#define MCTL_CHIP_SIZE			2048			//unit in Mb
#define MCTL_CAS				7
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			15
#define MCTL_BUS_WIDTH			32
#endif

#ifdef DDR3_16B8
//DDR3 128Mx16 (256M Byte)
#define MCTL_DDR_TYPE			3				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			8
#define MCTL_CHIP_SIZE			2048			//unit in Mb
#define MCTL_CAS				7
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			15
#define MCTL_BUS_WIDTH			16
#endif


#ifdef DDR3_32B
//DDR3 128Mx16 (256M Byte)
#define MCTL_DDR_TYPE			3				//1: DDR, 2: DDR2, 3: DDR3
#define MCTL_IO_WIDTH			16
#define MCTL_CHIP_SIZE			4096			//unit in Mb
#define MCTL_CAS				7
#define MCTL_BANK_SIZE			8
#define MCTL_COL_WIDTH			10
#define MCTL_ROW_WIDTH			15
#define MCTL_BUS_WIDTH			32
#endif



#endif  //MCTL_PAR_H
