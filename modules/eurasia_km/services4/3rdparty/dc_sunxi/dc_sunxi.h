/*************************************************************************/ /*!
@Title          SUNXI Linux display driver structures and prototypes
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  
*/ /**************************************************************************/
#ifndef __SUNXILFB_H__
#define __SUNXILFB_H__

#include <linux/version.h>

#include <asm/atomic.h>

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/mutex.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
#define	SUNXILFB_CONSOLE_LOCK()		console_lock()
#define	SUNXILFB_CONSOLE_UNLOCK()	console_unlock()
#else
#define	SUNXILFB_CONSOLE_LOCK()		acquire_console_sem()
#define	SUNXILFB_CONSOLE_UNLOCK()	release_console_sem()
#endif

#define unref__ __attribute__ ((unused))

typedef void *       SUNXILFB_HANDLE;

typedef bool SUNXILFB_BOOL, *SUNXILFB_PBOOL;
#define	SUNXILFB_FALSE false
#define SUNXILFB_TRUE true

typedef	atomic_t	SUNXILFB_ATOMIC_BOOL;

typedef atomic_t	SUNXILFB_ATOMIC_INT;

/* SUNXILFB buffer structure */
typedef struct SUNXILFB_BUFFER_TAG
{
	struct SUNXILFB_BUFFER_TAG	*psNext;
	struct SUNXILFB_DEVINFO_TAG	*psDevInfo;

	struct work_struct sWork;

	/* Position of this buffer in the virtual framebuffer */
	unsigned long		     	ulYOffset;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	IMG_SYS_PHYADDR              	sSysAddr;
	IMG_CPU_VIRTADDR             	sCPUVAddr;
	PVRSRV_SYNC_DATA            	*psSyncData;

	SUNXILFB_HANDLE      		hCmdComplete;
	unsigned long    		ulSwapInterval;
} SUNXILFB_BUFFER;

/* SUNXILFB swapchain structure */
typedef struct SUNXILFB_SWAPCHAIN_TAG
{
	/* Swap chain ID */
	unsigned int			uiSwapChainID;

	/* number of buffers in swapchain */
	unsigned long       		ulBufferCount;

	/* list of buffers in the swapchain */
	SUNXILFB_BUFFER     		*psBuffer;

	/* Swap chain work queue */
	struct workqueue_struct   	*psWorkQueue;

	/*
	 * Set if we didn't manage to wait for VSync on last swap,
	 * or if we think we need to wait for VSync on the next flip.
	 * The flag helps to avoid jitter when the screen is
	 * unblanked, by forcing an extended wait for VSync before
	 * attempting the next flip.
	 */
	SUNXILFB_BOOL			bNotVSynced;

	/* Previous number of blank events */
	int				iBlankEvents;

	/* Framebuffer Device ID for messages (e.g. printk) */
	unsigned int            	uiFBDevID;
} SUNXILFB_SWAPCHAIN;

typedef struct SUNXILFB_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;
	unsigned long       ulPhysicalWidthmm;
	unsigned long       ulPhysicalHeightmm;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	IMG_SYS_PHYADDR     sSysAddr;//system physical address
	IMG_CPU_VIRTADDR    sCPUVAddr;

	/* pixelformat of system/primary surface */
	PVRSRV_PIXEL_FORMAT ePixelFormat;

#if defined(CONFIG_DSSCOMP)
	SUNXILFB_BOOL		bIs2D;
	IMG_SYS_PHYADDR		*psPageList;
	struct ion_handle	*psIONHandle;
	IMG_UINT32			uiBytesPerPixel;
#endif
} SUNXILFB_FBINFO;

/* kernel device information structure */
typedef struct SUNXILFB_DEVINFO_TAG
{
	/* Framebuffer Device ID */
	unsigned int            uiFBDevID;

	/* PVR Device ID */
	unsigned int            uiPVRDevID;

	/* Swapchain create/destroy mutex */
	struct mutex		sCreateSwapChainMutex;

	/* system surface info */
	SUNXILFB_BUFFER          sSystemBuffer;

	/* jump table into PVR services */
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;
	
	/* jump table into DC */
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	/* fb info structure */
	SUNXILFB_FBINFO          sFBInfo;

	/* Only one swapchain supported by this device so hang it here */
	SUNXILFB_SWAPCHAIN      *psSwapChain;

	/* Swap chain ID */
	unsigned int		uiSwapChainID;

	/* True if PVR Services is flushing its command queues */
	SUNXILFB_ATOMIC_BOOL     sFlushCommands;

	/* pointer to linux frame buffer information structure */
	struct fb_info         *psLINFBInfo;

	/* Linux Framebuffer event notification block */
	struct notifier_block   sLINNotifBlock;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */

	/* Address of the surface being displayed */
	IMG_DEV_VIRTADDR	sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;

	/* Display format */
	DISPLAY_FORMAT          sDisplayFormat;
	
	/* Display dimensions */
	DISPLAY_DIMS            sDisplayDim;

	/* True if screen is blanked */
	SUNXILFB_ATOMIC_BOOL	sBlanked;

	/* Number of blank/unblank events */
	SUNXILFB_ATOMIC_INT	sBlankEvents;

#ifdef CONFIG_HAS_EARLYSUSPEND
	/* Set by early suspend */
	SUNXILFB_ATOMIC_BOOL	sEarlySuspendFlag;

	struct early_suspend    sEarlySuspend;
#endif

#if defined(SUPPORT_DRI_DRM)
	SUNXILFB_ATOMIC_BOOL     sLeaveVT;
#endif

}  SUNXILFB_DEVINFO;

#define	SUNXILFB_PAGE_SIZE 4096

/* DEBUG only printk */
#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "SUNXI Linux Display Driver"
#define	DRVNAME	"dc_sunxi"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

/*!
 *****************************************************************************
 * Error values
 *****************************************************************************/
typedef enum _SUNXILFB_ERROR_
{
	SUNXILFB_OK                             =  0,
	SUNXILFB_ERROR_GENERIC                  =  1,
	SUNXILFB_ERROR_OUT_OF_MEMORY            =  2,
	SUNXILFB_ERROR_TOO_FEW_BUFFERS          =  3,
	SUNXILFB_ERROR_INVALID_PARAMS           =  4,
	SUNXILFB_ERROR_INIT_FAILURE             =  5,
	SUNXILFB_ERROR_CANT_REGISTER_CALLBACK   =  6,
	SUNXILFB_ERROR_INVALID_DEVICE           =  7,
	SUNXILFB_ERROR_DEVICE_REGISTER_FAILED   =  8,
	SUNXILFB_ERROR_SET_UPDATE_MODE_FAILED   =  9
} SUNXILFB_ERROR;

typedef enum _SUNXILFB_UPDATE_MODE_
{
	SUNXILFB_UPDATE_MODE_UNDEFINED			= 0,
	SUNXILFB_UPDATE_MODE_MANUAL			= 1,
	SUNXILFB_UPDATE_MODE_AUTO			= 2,
	SUNXILFB_UPDATE_MODE_DISABLED			= 3
} SUNXILFB_UPDATE_MODE;

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

SUNXILFB_ERROR SUNXILFBInit(void);
SUNXILFB_ERROR SUNXILFBDeInit(void);

/* OS Specific APIs */
SUNXILFB_DEVINFO *SUNXILFBGetDevInfoPtr(unsigned uiFBDevID);
unsigned SUNXILFBMaxFBDevIDPlusOne(void);
void *SUNXILFBAllocKernelMem(unsigned long ulSize);
void SUNXILFBFreeKernelMem(void *pvMem);
SUNXILFB_ERROR SUNXILFBGetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
SUNXILFB_ERROR SUNXILFBCreateSwapQueue (SUNXILFB_SWAPCHAIN *psSwapChain);
void SUNXILFBDestroySwapQueue(SUNXILFB_SWAPCHAIN *psSwapChain);
void SUNXILFBInitBufferForSwap(SUNXILFB_BUFFER *psBuffer);
void SUNXILFBSwapHandler(SUNXILFB_BUFFER *psBuffer);
void SUNXILFBQueueBufferForSwap(SUNXILFB_SWAPCHAIN *psSwapChain, SUNXILFB_BUFFER *psBuffer);
void SUNXILFBFlip(SUNXILFB_DEVINFO *psDevInfo, SUNXILFB_BUFFER *psBuffer);
SUNXILFB_BOOL SUNXILFBSetUpdateMode(SUNXILFB_DEVINFO *psDevInfo, SUNXILFB_UPDATE_MODE eMode);
SUNXILFB_BOOL SUNXILFBWaitForVSync(SUNXILFB_DEVINFO *psDevInfo);
SUNXILFB_BOOL SUNXILFBManualSync(SUNXILFB_DEVINFO *psDevInfo);
SUNXILFB_BOOL SUNXILFBCheckModeAndSync(SUNXILFB_DEVINFO *psDevInfo);
SUNXILFB_ERROR SUNXILFBUnblankDisplay(SUNXILFB_DEVINFO *psDevInfo);
SUNXILFB_ERROR SUNXILFBEnableLFBEventNotification(SUNXILFB_DEVINFO *psDevInfo);
SUNXILFB_ERROR SUNXILFBDisableLFBEventNotification(SUNXILFB_DEVINFO *psDevInfo);
void SUNXILFBCreateSwapChainLockInit(SUNXILFB_DEVINFO *psDevInfo);
void SUNXILFBCreateSwapChainLockDeInit(SUNXILFB_DEVINFO *psDevInfo);
void SUNXILFBCreateSwapChainLock(SUNXILFB_DEVINFO *psDevInfo);
void SUNXILFBCreateSwapChainUnLock(SUNXILFB_DEVINFO *psDevInfo);
void SUNXILFBAtomicBoolInit(SUNXILFB_ATOMIC_BOOL *psAtomic, SUNXILFB_BOOL bVal);
void SUNXILFBAtomicBoolDeInit(SUNXILFB_ATOMIC_BOOL *psAtomic);
void SUNXILFBAtomicBoolSet(SUNXILFB_ATOMIC_BOOL *psAtomic, SUNXILFB_BOOL bVal);
SUNXILFB_BOOL SUNXILFBAtomicBoolRead(SUNXILFB_ATOMIC_BOOL *psAtomic);
void SUNXILFBAtomicIntInit(SUNXILFB_ATOMIC_INT *psAtomic, int iVal);
void SUNXILFBAtomicIntDeInit(SUNXILFB_ATOMIC_INT *psAtomic);
void SUNXILFBAtomicIntSet(SUNXILFB_ATOMIC_INT *psAtomic, int iVal);
int SUNXILFBAtomicIntRead(SUNXILFB_ATOMIC_INT *psAtomic);
void SUNXILFBAtomicIntInc(SUNXILFB_ATOMIC_INT *psAtomic);

#if defined(DEBUG)
void SUNXILFBPrintInfo(SUNXILFB_DEVINFO *psDevInfo);
#else
#define	SUNXILFBPrintInfo(psDevInfo)
#endif

#endif /* __SUNXILFB_H__ */

/******************************************************************************
 End of file (dc_sunxi.h)
******************************************************************************/

