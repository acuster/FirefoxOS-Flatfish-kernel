#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/serial.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/kfifo.h>

#include "sw_dongle.h"

static int sw_dongle_remote_wakeup(struct usb_device *udev, u32 set)
{
    int status = 0;

    printk("sw_dongle_remote_wakeup: %s remote wakeup\n", (set ? "set" : "clear"));

    if(set){
        /* set remote wakeup */
        status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_DEVICE,
				USB_DEVICE_REMOTE_WAKEUP, 0,
				NULL, 0,
				USB_CTRL_SET_TIMEOUT);
		if(status){
		    printk("err: set remote wakeup failed, status=%d\n", status);
		    return -1;
		}
    }else{
        /* clear remote wakeup */
        status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
                USB_REQ_CLEAR_FEATURE, USB_RECIP_DEVICE,
                USB_DEVICE_REMOTE_WAKEUP, 0,
                NULL, 0,
                USB_CTRL_SET_TIMEOUT);
        if(status){
		    printk("err: suspend port failed, status=%d\n", status);
		    return -1;
		}
    }

	return 0;
}

static int sw_dongle_clear_halt(struct usb_device *udev, unsigned int pipe)
{
    int status = 0;
	int endp = usb_pipeendpoint(pipe);

    printk("sw_dongle_clear_halt: ep%d_%s\n", endp, (usb_pipein(pipe) ? "in" : "out"));

    /* clear halt */
    status = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
			USB_ENDPOINT_HALT, (usb_pipein(pipe)) | endp,
			NULL, 0,
			USB_CTRL_SET_TIMEOUT);
	if(status){
	    printk("err: sw_dongle_clear_halt failed, ep%d_%s, status=%d\n", endp, (usb_pipein(pipe) ? "in" : "out"), status);
	    return -1;
	}

	return 0;
}

int zte_a355_init(struct usb_interface *intf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_endpoint_descriptor *endpoint;
	unsigned int pipe = 0;
    int i = 0;

    /* set remote wakeup */
    sw_dongle_remote_wakeup(udev, 1);

    /* clear halt */
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; ++i) {
		endpoint = &intf->cur_altsetting->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(endpoint)) {
            pipe = usb_rcvbulkpipe(udev, usb_endpoint_num(endpoint));
        }else if (usb_endpoint_is_bulk_out(endpoint)) {
            pipe = usb_sndbulkpipe(udev, usb_endpoint_num(endpoint));
        }else{
            pipe = 0;
        }

        sw_dongle_clear_halt(udev, pipe);
    }

    return 0;
}

int zte_mu350_init(struct usb_interface *intf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_endpoint_descriptor *endpoint;
	unsigned int pipe = 0;
    int i = 0;

    /* clear halt */
	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; ++i) {
		endpoint = &intf->cur_altsetting->endpoint[i].desc;

        if (usb_endpoint_is_bulk_in(endpoint)) {
            pipe = usb_rcvbulkpipe(udev, usb_endpoint_num(endpoint));
        }else if (usb_endpoint_is_bulk_out(endpoint)) {
            pipe = usb_sndbulkpipe(udev, usb_endpoint_num(endpoint));
        }else{
            pipe = 0;
        }

        sw_dongle_clear_halt(udev, pipe);
    }

    return 0;
}

