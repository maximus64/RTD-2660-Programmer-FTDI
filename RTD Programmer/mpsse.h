#ifndef MPSSE_H
#define MPSSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <libusb-1.0/libusb.h>

/* Mode flags */
#define MODE_NEG_EDGE_WRITE (1<<0)
#define MODE_BIT            (1<<1)
#define MODE_NEG_EDGE_READ  (1<<2)
#define MODE_LSB            (1<<3)
#define MODE_WRITE_TDI      (1<<4)
#define MODE_READ_TDO       (1<<5)
#define MODE_WRITE_TMS      (1<<6)

#define FTDI_DEVICE_OUT_REQTYPE (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)
#define FTDI_DEVICE_IN_REQTYPE (0x80 | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)

#define BITMODE_MPSSE 0x02

#define SIO_RESET_REQUEST             0x00
#define SIO_SET_LATENCY_TIMER_REQUEST 0x09
#define SIO_GET_LATENCY_TIMER_REQUEST 0x0A
#define SIO_SET_BITMODE_REQUEST       0x0B

#define SIO_RESET_SIO 0
#define SIO_RESET_PURGE_RX 1
#define SIO_RESET_PURGE_TX 2

enum ftdi_chip_type {
	TYPE_FT2232C,
	TYPE_FT2232H,
	TYPE_FT4232H,
	TYPE_FT232H,
};

struct mpsse_ctx {
	libusb_context *usb_ctx;
	libusb_device_handle *usb_dev;

	unsigned int usb_write_timeout;
	unsigned int usb_read_timeout;
	uint8_t in_ep;
	uint8_t out_ep;
	uint16_t max_packet_size;
	uint16_t index;
	uint8_t interface;
	enum ftdi_chip_type type;
};


void mpsse_close(struct mpsse_ctx *ctx);
struct mpsse_ctx *mpsse_open(uint16_t vid, uint16_t pid, int channel);
void mpsse_purge(struct mpsse_ctx *ctx);

int mpsse_read(struct mpsse_ctx *ctx, uint8_t *buf, int len);
int mpsse_write(struct mpsse_ctx *ctx, uint8_t *buf, int len);

#endif
