#include "mpsse.h"

#include "helper.h"

/* Helper to open a libusb device that matches vid, pid, product string and/or serial string.
 * Set any field to 0 as a wildcard. If the device is found true is returned, with ctx containing
 * the already opened handle. ctx->interface must be set to the desired interface (channel) number
 * prior to calling this function. */
static bool open_matching_device(struct mpsse_ctx *ctx, uint16_t vid, uint16_t pid)
{
	libusb_device **list;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config0;
	int err;
	bool found = false;
	ssize_t cnt = libusb_get_device_list(ctx->usb_ctx, &list);
	int i;

	if (cnt < 0)
		LOG_ERROR("libusb_get_device_list() failed with %s", libusb_error_name(cnt));

	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];

		err = libusb_get_device_descriptor(device, &desc);
		if (err != LIBUSB_SUCCESS) {
			LOG_ERROR("libusb_get_device_descriptor() failed with %s", libusb_error_name(err));
			continue;
		}

		if (vid != desc.idVendor)
			continue;
		if (pid != desc.idProduct)
			continue;

		err = libusb_open(device, &ctx->usb_dev);
		if (err != LIBUSB_SUCCESS) {
			LOG_ERROR("libusb_open() failed with %s",
				  libusb_error_name(err));
			continue;
		}

		found = true;
		break;
	}

	libusb_free_device_list(list, 1);

	if (!found) {
		LOG_ERROR("no device found");
		return false;
	}

	err = libusb_get_config_descriptor(libusb_get_device(ctx->usb_dev), 0, &config0);
	if (err != LIBUSB_SUCCESS) {
		LOG_ERROR("libusb_get_config_descriptor() failed with %s", libusb_error_name(err));
		libusb_close(ctx->usb_dev);
		return false;
	}

	/* Make sure the first configuration is selected */
	int cfg;
	err = libusb_get_configuration(ctx->usb_dev, &cfg);
	if (err != LIBUSB_SUCCESS) {
		LOG_ERROR("libusb_get_configuration() failed with %s", libusb_error_name(err));
		goto error;
	}

	if (desc.bNumConfigurations > 0 && cfg != config0->bConfigurationValue) {
		err = libusb_set_configuration(ctx->usb_dev, config0->bConfigurationValue);
		if (err != LIBUSB_SUCCESS) {
			LOG_ERROR("libusb_set_configuration() failed with %s", libusb_error_name(err));
			goto error;
		}
	}

	/* Try to detach ftdi_sio kernel module */
	err = libusb_detach_kernel_driver(ctx->usb_dev, ctx->interface);
	if (err != LIBUSB_SUCCESS && err != LIBUSB_ERROR_NOT_FOUND
			&& err != LIBUSB_ERROR_NOT_SUPPORTED) {
		LOG_ERROR("libusb_detach_kernel_driver() failed with %s", libusb_error_name(err));
		goto error;
	}

	err = libusb_claim_interface(ctx->usb_dev, ctx->interface);
	if (err != LIBUSB_SUCCESS) {
		LOG_ERROR("libusb_claim_interface() failed with %s", libusb_error_name(err));
		goto error;
	}

	/* Reset FTDI device */
	err = libusb_control_transfer(ctx->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
			SIO_RESET_REQUEST, SIO_RESET_SIO,
			ctx->index, NULL, 0, ctx->usb_write_timeout);
	if (err < 0) {
		LOG_ERROR("failed to reset FTDI device: %s", libusb_error_name(err));
		goto error;
	}

	switch (desc.bcdDevice) {
	case 0x500:
		ctx->type = TYPE_FT2232C;
		LOG_DEBUG("Detected FT2232C chipset");
		break;
	case 0x700:
		ctx->type = TYPE_FT2232H;
		LOG_DEBUG("Detected TYPE_FT2232H chipset");
		break;
	case 0x800:
		ctx->type = TYPE_FT4232H;
		LOG_DEBUG("Detected TYPE_FT4232H chipset");
		break;
	case 0x900:
		ctx->type = TYPE_FT232H;
		LOG_DEBUG("Detected TYPE_FT232H chipset");
		break;
	default:
		LOG_ERROR("unsupported FTDI chip type: 0x%04x", desc.bcdDevice);
		goto error;
	}

	/* Determine maximum packet size and endpoint addresses */
	if (!(desc.bNumConfigurations > 0 && ctx->interface < config0->bNumInterfaces
			&& config0->interface[ctx->interface].num_altsetting > 0))
		goto desc_error;

	const struct libusb_interface_descriptor *descriptor;
	descriptor = &config0->interface[ctx->interface].altsetting[0];
	if (descriptor->bNumEndpoints != 2)
		goto desc_error;

	ctx->in_ep = 0;
	ctx->out_ep = 0;

	for (i = 0; i < descriptor->bNumEndpoints; i++) {
		if (descriptor->endpoint[i].bEndpointAddress & 0x80) {
			ctx->in_ep = descriptor->endpoint[i].bEndpointAddress;
			ctx->max_packet_size =
					descriptor->endpoint[i].wMaxPacketSize;
		} else {
			ctx->out_ep = descriptor->endpoint[i].bEndpointAddress;
		}
	}

	if (ctx->in_ep == 0 || ctx->out_ep == 0)
		goto desc_error;

	libusb_free_config_descriptor(config0);
	return true;

desc_error:
	LOG_ERROR("unrecognized USB device descriptor");
error:
	libusb_free_config_descriptor(config0);
	libusb_close(ctx->usb_dev);
	return false;
}


struct mpsse_ctx *mpsse_open(uint16_t vid, uint16_t pid, int channel)
{
	struct mpsse_ctx *ctx = calloc(1, sizeof(*ctx));
	int err;

	if (!ctx)
		return 0;

	ctx->interface = channel;
	ctx->index = channel + 1;
	ctx->usb_read_timeout = 50000;
	ctx->usb_write_timeout = 50000;

	err = libusb_init(&ctx->usb_ctx);
	if (err != LIBUSB_SUCCESS) {
		LOG_ERROR("libusb_init() failed with %s", libusb_error_name(err));
		goto error;
	}

	if (!open_matching_device(ctx, vid, pid)) {
		/* Four hex digits plus terminating zero each */
		char vidstr[5];
		char pidstr[5];
		LOG_ERROR("unable to open ftdi device with vid %s, pid %s",
				vid ? sprintf(vidstr, "%04x", vid), vidstr : "*",
				pid ? sprintf(pidstr, "%04x", pid), pidstr : "*");
		ctx->usb_dev = 0;
		goto error;
	}

	//Set latency timer to 255 ms. 
	err = libusb_control_transfer(ctx->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
			SIO_SET_LATENCY_TIMER_REQUEST, 255, ctx->index, NULL, 0,
			ctx->usb_write_timeout);
	if (err < 0) {
		LOG_ERROR("unable to set latency timer: %s", libusb_error_name(err));
		goto error;
	}

	err = libusb_control_transfer(ctx->usb_dev,
			FTDI_DEVICE_OUT_REQTYPE,
			SIO_SET_BITMODE_REQUEST,
			0x0b | (BITMODE_MPSSE << 8),
			ctx->index,
			NULL,
			0,
			ctx->usb_write_timeout);
	if (err < 0) {
		LOG_ERROR("unable to set MPSSE bitmode: %s", libusb_error_name(err));
		goto error;
	}

	mpsse_purge(ctx);

	return ctx;
error:
	mpsse_close(ctx);
	return 0;
}

int mpsse_write(struct mpsse_ctx *ctx, uint8_t *buf, int len)
{
	int err;
	int trans;

	err = libusb_bulk_transfer(ctx->usb_dev,
		ctx->out_ep,
		buf,
		len,
		&trans,
		ctx->usb_write_timeout);

	if (err < 0) {
		LOG_ERROR("Unable to write data to EP: %s", libusb_error_name(err));
		return -1;
	}

	return trans;
}

int mpsse_read(struct mpsse_ctx *ctx, uint8_t *buf, int len)
{
	int err, trans, trans2, data_left;
	uint8_t *temp_buf;

	//This FTDI chip sometime it doesn't send out the data package till we poll it for several time weird
	while(1)
	{
		//start to read
		err = libusb_bulk_transfer(ctx->usb_dev,
			ctx->in_ep,
			buf,
			len,
			&trans,
			ctx->usb_write_timeout);

		if (err < 0) {
			LOG_ERROR("Unable to read data at EP: %s", libusb_error_name(err));
			return -1;
		}

		/* 
		 * sometime the ftdi chip only send back part of the package
		 * need check the status byte to see if there any packge comming up
		 * 0x32 0x00 - more data comming
		 * 0x32 0x60 - that it end of package
		 */
		if(len > trans && trans >= 2 && buf[1] == 0x00)
		{
			//LOG_DEBUG("Not the end of the package. More data comming. status=0x%02x%02x", buf[0], buf[1]);

			data_left = (len - trans) + 2; //2 status bytes
			temp_buf = malloc(data_left);

			while(buf[1] == 0x00 && len > trans)
			{
				//read ep
				err = libusb_bulk_transfer(ctx->usb_dev,
					ctx->in_ep,
					temp_buf,
					data_left,
					&trans2,
					ctx->usb_write_timeout);

				if (err < 0) {
					LOG_ERROR("Unable to read data at EP: %s", libusb_error_name(err));
					return -1;
				}

				//check package len. Expect minimum 2 bytes status
				if(trans2 >=2)
				{
					//update status bytes
					buf[0] = temp_buf[0];
					buf[1] = temp_buf[1];
					//LOG_DEBUG("Got data package. status=0x%02x%02x", buf[0], buf[1]);

					if(trans+(trans2-2) > len)
					{
						LOG_ERROR("Package size too long.");
						return -1;
					}

					//copy over data package
					memcpy(buf+trans, temp_buf+2, trans2-2);

					trans += trans2-2;
				}
				else
				{
					LOG_ERROR("Unexpected data package length");
					return -1;
				}
			}

			free(temp_buf);
		}

		if(trans != len)
		{
			//hexdump(buf, trans);
			continue;//LOG_DEBUG("Only got back %d bytes from the device. Asking for %d. Retrying...", trans, len);
		}
		else
			break;
	}

	return trans;
}

void mpsse_purge(struct mpsse_ctx *ctx)
{
	int err;

	err = libusb_control_transfer(ctx->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_RESET_REQUEST,
			SIO_RESET_PURGE_RX, ctx->index, NULL, 0, ctx->usb_write_timeout);
	if (err < 0) {
		LOG_ERROR("unable to purge ftdi rx buffers: %s", libusb_error_name(err));
		return;
	}

	err = libusb_control_transfer(ctx->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_RESET_REQUEST,
			SIO_RESET_PURGE_TX, ctx->index, NULL, 0, ctx->usb_write_timeout);
	if (err < 0) {
		LOG_ERROR("unable to purge ftdi tx buffers: %s", libusb_error_name(err));
		return;
	}
}

void mpsse_close(struct mpsse_ctx *ctx)
{
	if (ctx->usb_dev)
		libusb_close(ctx->usb_dev);
	if (ctx->usb_ctx)
		libusb_exit(ctx->usb_ctx);

	free(ctx);
}
