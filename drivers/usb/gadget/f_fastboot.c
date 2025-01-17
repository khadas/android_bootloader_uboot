// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 */
#include <config.h>
#include <common.h>
#include <errno.h>
#include <fastboot.h>
#include <malloc.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/compiler.h>
#include <g_dnl.h>
#include <partition_table.h>
#include <amlogic/aml_rollback.h>

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

#define DEVICE_SERIAL	"1234567890"
#define DEVICE_NAME "USB fastboot gadget"

#define EP_BUFFER_SIZE			4096
/*
 * EP_BUFFER_SIZE must always be an integral multiple of maxpacket size
 * (64 or 512 or 1024), else we break on certain controllers like DWC3
 * that expect bulk OUT requests to be divisible by maxpacket size.
 */

#ifndef CONFIG_USB_GADGET_CRG
extern void f_dwc_otg_pullup(int is_on);
#else
extern int crg_gadget_stop(struct usb_gadget *g);
#endif

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength            = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType    = USB_DT_ENDPOINT,
	.bEndpointAddress   = USB_DIR_IN,
	.bmAttributes       = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize     = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(64),
};

static struct usb_endpoint_descriptor hs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fb_fs_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&fs_ep_out,
};

static struct usb_descriptor_header *fb_hs_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&hs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

static struct usb_endpoint_descriptor *
fb_ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *fs,
	    struct usb_endpoint_descriptor *hs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;

	if ( (busy_flag == 1) && fastboot_func) {
		struct usb_ep* out_ep = fastboot_func->out_ep;
		struct usb_request* out_req = fastboot_func->out_req;
		rx_handler_command(out_ep, out_req);
		return;
	}
	if (!status)
		return;
	printf("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	const char *s;
	const char *board_name;

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	f_fb->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

	f->descriptors = fb_fs_function;

	if (gadget_is_dualspeed(gadget)) {
		/* Assume endpoint addresses are the same for both speeds */
		hs_ep_in.bEndpointAddress = fs_ep_in.bEndpointAddress;
		hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;
		/* copy HS descriptors */
		f->hs_descriptors = fb_hs_function;
	}

	s = env_get("serial#");
	if (s) {
		printf("serial num: %s\n", s);
		g_dnl_set_serialnumber((char *)s);
	} else
		g_dnl_set_serialnumber(DEVICE_SERIAL);

	board_name = env_get("board");
	if (board_name)
		g_dnl_set_productname((char *)board_name);
	else
		g_dnl_set_productname(DEVICE_NAME);

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);

	if (f_fb->out_req) {
		free(f_fb->out_req->buf);
		usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
		f_fb->out_req = NULL;
	}
	if (f_fb->in_req) {
		free(f_fb->in_req->buf);
		usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
		f_fb->in_req = NULL;
	}
}

static struct usb_request *fastboot_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	return req;
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int ret;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	const struct usb_endpoint_descriptor *d;

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	d = fb_ep_desc(gadget, &fs_ep_out, &hs_ep_out);
	ret = usb_ep_enable(f_fb->out_ep, d);
	if (ret) {
		puts("failed to enable out ep\n");
		return ret;
	}

	f_fb->out_req = fastboot_start_ep(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->out_req->complete = rx_handler_command;

	d = fb_ep_desc(gadget, &fs_ep_in, &hs_ep_in);
	ret = usb_ep_enable(f_fb->in_ep, d);
	if (ret) {
		puts("failed to enable in ep\n");
		goto err;
	}

	f_fb->in_req = fastboot_start_ep(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->in_req->complete = fastboot_complete;

	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static int  fastboot_setup(struct usb_function *f,
	const struct usb_ctrlrequest *ctrl)
{
	int value = -EOPNOTSUPP;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	/* composite driver infrastructure handles everything; interface
	 * activation uses set_alt().
	 */
	if (((ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT)
		&& (ctrl->bRequest == USB_REQ_CLEAR_FEATURE)
		&& (ctrl->wValue== USB_ENDPOINT_HALT)) {
		switch (ctrl->wIndex & 0xfe) {
		case USB_DIR_OUT:
			value = ctrl->wLength;
			usb_ep_clear_halt(f_fb->out_ep);
			break;

		case USB_DIR_IN:
			value = ctrl->wLength;
			usb_ep_clear_halt(f_fb->in_ep);
			break;
		default:
			printf("unknown usb_ctrlrequest\n");
			break;
		}
	}

	return value;
}

static int fastboot_add(struct usb_configuration *c)
{
	struct f_fastboot *f_fb;
	int status;

	debug("%s: cdev: 0x%p\n", __func__, c->cdev);

	if (fastboot_func == NULL) {
		f_fb = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_fb));
		if (!f_fb)
			return -ENOMEM;

		fastboot_func = f_fb;
		memset(f_fb, 0, sizeof(*f_fb));
	} else {
		f_fb = fastboot_func;
	}

	f_fb->usb_function.name = "f_fastboot";
	f_fb->usb_function.bind = fastboot_bind;
	f_fb->usb_function.unbind = fastboot_unbind;
	f_fb->usb_function.set_alt = fastboot_set_alt;
	f_fb->usb_function.disable = fastboot_disable;
	f_fb->usb_function.strings = fastboot_strings;
	f_fb->usb_function.setup = fastboot_setup;

	status = usb_add_function(c, &f_fb->usb_function);
	if (status) {
		free(f_fb);
		fastboot_func = NULL;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;

	usb_ep_dequeue(fastboot_func->in_ep, in_req);

	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

void check_fastboot_step(void)
{
	char *fastboot_step = env_get("fastboot_step");
	char *gpt_mode = env_get("gpt_mode");
	char *nocs_mode = env_get("nocs_mode");

	if (fastboot_step && (strcmp(fastboot_step, "2") == 0)) {
		//come to here, means new burn bootloader.img is OK, reset env
		printf("new burn bootloader.img is OK, write other bootloader\n");

		if ((gpt_mode && !strcmp(gpt_mode, "true")) ||
			(nocs_mode && !strcmp(nocs_mode, "true"))) {
			printf("gpt or disable user bootloader mode\n");
			run_command("copy_slot_bootable 2 1", 0);
		} else {
			printf("normal mode\n");
			run_command("copy_slot_bootable 1 0", 0);
			run_command("copy_slot_bootable 1 2", 0);
		}

		env_set("fastboot_step", "0");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p fastboot_step;", 0);
#else
		run_command("defenv_reserve;setenv fastboot_step 0;saveenv;", 0);
#endif
	}
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	check_fastboot_step();
#ifndef CONFIG_USB_GADGET_CRG
	f_dwc_otg_pullup(0);
#else
	crg_gadget_stop(NULL);
#endif

	do_reset(NULL, 0, 0, NULL);
}

static void compl_do_reboot_bootloader(struct usb_ep *ep, struct usb_request *req)
{
	check_fastboot_step();
#ifndef CONFIG_USB_GADGET_CRG
	f_dwc_otg_pullup(0);
#else
	crg_gadget_stop(NULL);
#endif
	if (dynamic_partition)
		run_command("reboot bootloader", 0);
	else
		run_command("reboot fastboot", 0);
}

static void compl_do_reboot_fastboot(struct usb_ep *ep, struct usb_request *req)
{
	check_fastboot_step();
#ifndef CONFIG_USB_GADGET_CRG
	f_dwc_otg_pullup(0);
#else
	crg_gadget_stop(NULL);
#endif

	run_command("reboot fastboot", 0);
}

static void compl_do_reboot_recovery(struct usb_ep *ep, struct usb_request *req)
{
	check_fastboot_step();
#ifndef CONFIG_USB_GADGET_CRG
	f_dwc_otg_pullup(0);
#endif
	run_command("reboot recovery", 0);
}

static unsigned int rx_bytes_expected(struct usb_ep *ep)
{
	int rx_remain = fastboot_data_remaining();
	unsigned int rem;
	unsigned int maxpacket = ep->maxpacket;

	if (rx_remain <= 0)
		return 0;
	else if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;

	/*
	 * Some controllers e.g. DWC3 don't like OUT transfers to be
	 * not ending in maxpacket boundary. So just make them happy by
	 * always requesting for integral multiple of maxpackets.
	 * This shouldn't bother controllers that don't care about it.
	 */
	rem = rx_remain % maxpacket;
	if (rem > 0)
		rx_remain = rx_remain + (maxpacket - rem);

	return rx_remain;
}

static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[FASTBOOT_RESPONSE_LEN] = {0};
	unsigned int transfer_size = fastboot_data_remaining();
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	fastboot_data_download(buffer, transfer_size, response);
	if (response[0]) {
		fastboot_tx_write_str(response);
	} else if (!fastboot_data_remaining()) {
		fastboot_data_complete(response);

		/*
		 * Reset global transfer variable
		 */
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;
		fastboot_tx_write_str(response);
	} else {
		req->length = rx_bytes_expected(ep);
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void tx_handler_mread(struct usb_ep *inep, struct usb_request *inreq)
{
	const unsigned int transfer_size = inreq->actual;

	if (inreq->status != 0) {
		printf("in req Bad status: %d\n", inreq->status);
		return;
	}

	if (fastboot_func->in_req != inreq) {
		printf("exception, bogus req\n");
		return;
	}

	fastboot_readInfo.transferredBytes += transfer_size;

	/* Check if transfer is done */
	if (fastboot_readInfo.transferredBytes >= fastboot_readInfo.totalBytes) {
		printf("mread 0x%x bytes end\n", fastboot_readInfo.transferredBytes);
		/*write ended and return to receive command*/
		inreq->complete = fastboot_complete;
		inreq->length = EP_BUFFER_SIZE;
		if (fastboot_readInfo.priv)
			inreq->buf = (char *)fastboot_readInfo.priv;
		//should return to rx next command
		fastboot_tx_write_str("OKAY");
	} else {
		const unsigned int leftLen = fastboot_readInfo.totalBytes
			- fastboot_readInfo.transferredBytes;

		if (leftLen > EP_BUFFER_SIZE)
			inreq->length = EP_BUFFER_SIZE;
		else
			inreq->length = leftLen;
		inreq->buf += transfer_size;//remove copy

		inreq->actual = 0;
		usb_ep_queue(inep, inreq, 0);
	}
}

static void do_exit_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	g_dnl_trigger_detach();
}

static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	fastboot_boot();
	do_exit_on_complete(ep, req);
}

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char cmdbuf[256];
	char response[FASTBOOT_RESPONSE_LEN] = {0};
	int cmd = -1;

	strncpy(cmdbuf, req->buf, 255);

	if (req->status != 0 || req->length == 0)
		return;

	if (req->actual < req->length) {
		cmdbuf[req->actual] = '\0';
		cmd = fastboot_handle_command(cmdbuf, response);
	} else {
		pr_err("buffer overflow");
		fastboot_fail("buffer overflow", response);
	}
	if (!strncmp("DATA", response, 4)) {
		if (cmd == 11) {
			printf("come to fetch code\n");
			struct usb_ep *inep = fastboot_func->in_ep;
			struct usb_request *inreq = fastboot_func->in_req;
			int ret;
			const unsigned int leftLen = fastboot_readInfo.totalBytes
				- fastboot_readInfo.transferredBytes;

			if (!fastboot_readInfo.priv)
				fastboot_readInfo.priv = inreq->buf;

			inreq->buf = (void *)CONFIG_FASTBOOT_BUF_ADDR;//to remove copy
			inreq->complete = tx_handler_mread;
			if (leftLen > EP_BUFFER_SIZE)
				inreq->length = EP_BUFFER_SIZE;
			else
				inreq->length = leftLen;

			ret = usb_ep_queue(inep, inreq, 0);
			if (ret)
				printf("Error %d on queue\n", ret);
		} else {
			req->complete = rx_handler_dl_image;
			req->length = rx_bytes_expected(ep);
		}
	}

	fastboot_tx_write_str(response);

	if (!strncmp("OKAY", response, 4)) {
		switch (cmd) {
		case FASTBOOT_COMMAND_BOOT:
			fastboot_func->in_req->complete = do_bootm_on_complete;
			break;

		case FASTBOOT_COMMAND_CONTINUE:
			fastboot_func->in_req->complete = do_exit_on_complete;
			break;

		case FASTBOOT_COMMAND_REBOOT:
			fastboot_func->in_req->complete = compl_do_reset;
			break;
		case FASTBOOT_COMMAND_REBOOT_BOOTLOADER:
			fastboot_func->in_req->complete = compl_do_reboot_bootloader;
			break;
		case FASTBOOT_COMMAND_REBOOT_FASTBOOT:
			fastboot_func->in_req->complete = compl_do_reboot_fastboot;
			break;
		case FASTBOOT_COMMAND_REBOOT_RECOVERY:
			fastboot_func->in_req->complete = compl_do_reboot_recovery;
			break;
		}
	}

	if (busy_flag == 0) {
		*cmdbuf = '\0';
		req->actual = 0;
		usb_ep_queue(ep, req, 0);
	}
}

