#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>

#define EP_IN 0x84
#define EP_OUT 0x03

#define VID 0x2717
#define PID 0xff48

static void log_device(libusb_device_handle *dev_handle);
static void log_configuration(libusb_device_handle *dev_handle);
static void log_interface(const struct libusb_interface *intf);
static void log_endpoint(const struct libusb_endpoint_descriptor *desc, int cnt);

void main() {

	libusb_context *cntxt = NULL;
	libusb_device_handle *dev_handle = NULL;

	if (libusb_init(&cntxt) < 0) {
		printf("Failed init usb context.\n");
		return;
	}

	int status = 0;

	do {

		printf("good job!\n");

		libusb_set_debug(cntxt, LIBUSB_LOG_LEVEL_INFO);

		dev_handle = libusb_open_device_with_vid_pid(cntxt, VID, PID);
		if (NULL == dev_handle) {
			printf("Failed open device.\n");
			break;
		}

		printf("Open device success.\n");
		log_device(dev_handle);

		if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
			printf("Kernel driver active.\n");
			if (libusb_detach_kernel_driver(dev_handle, 0) == 0) {
				printf("Kernel driver detached.\n");
			}
		}

		status = libusb_claim_interface(dev_handle, 2);
		if (status < 0) {
			printf("Failed claim interface:%s\n", libusb_strerror(status));
			break;
		}

		short code = 0x1001;
		char buff[30];
		memset(buff, 0, 30);
		memcpy(buff, &code, sizeof(code));

		int len;
		status = libusb_bulk_transfer(dev_handle,
				EP_OUT,
				(char *)&buff,
				sizeof(buff),
				&len,
				1000);
		if (status < 0) {
			printf("write error:%s\n", libusb_strerror(status));
			break;
		}
		printf("send %d bytes.\n", len);

		char read_buf[128];
		status = libusb_bulk_transfer(dev_handle,
				EP_IN,
				read_buf,
				128,
				&len,
				1000);
		if (status < 0) {
			printf("Failed read GetDeviceInfo response:%s.\n", libusb_strerror(status));
			break;
		}
		printf("read %d bytes.\n", len);


	} while (0);


	if (NULL != dev_handle) {
		status = libusb_release_interface(dev_handle, 0);
		if (status < 0) {
			printf("Failed release interface:%s\n", libusb_strerror(status));
		}

		libusb_attach_kernel_driver(dev_handle, 0);
		libusb_close(dev_handle);
		dev_handle = NULL;
	}

	if (NULL != cntxt) {
		libusb_exit(cntxt);
		cntxt = NULL;
	}

}

static void log_device(libusb_device_handle *dev_handle) {
	if (NULL == dev_handle) {
		return;
	}

	int config;
	int status = libusb_get_configuration(dev_handle, &config);
	if (status != 0) {
		printf ("Failed get config:%s\n", libusb_strerror(status));
	} else {
		printf("current usb configuration:%d\n", config);
	}

	log_configuration(dev_handle);
}

static void log_configuration(libusb_device_handle *dev_handle) {
	if (NULL == dev_handle) {
		return;
	}

	libusb_device *dev = libusb_get_device(dev_handle);
	if (NULL == dev) {
		printf("Failed get device.\n");
		return;
	}

	struct libusb_device_descriptor desc;
	int status = libusb_get_device_descriptor(dev, &desc);
	if (status != 0) {
		printf("Failed get device descriptor.\n");
		return;
	}

	uint16_t version = desc.bcdUSB;
	switch (version) {
		case 0x0100:
			printf("USB 1.0\n");
			break;

		case 0x0110:
			printf("USB 1.1\n");
			break;

		case 0x0200:
			printf("USB 2.0\n");
			break;

		case 0x0220:
			printf("USB 2.2\n");
			break;

		default:
			printf("Unknown version:0x%4x\n", version);
	}

	uint8_t numConf = desc.bNumConfigurations;
	printf("Possible configurations:%d\n", numConf);

	struct libusb_config_descriptor *config = NULL;
	status = libusb_get_active_config_descriptor(dev, &config);
	if (status != 0) {
		printf("Failed get active config descriptor.\n");
		return;
	}
	printf("interface num:%d\n", config->bNumInterfaces);	
	printf("configurations value:%d\n", config->bConfigurationValue);
	printf("configurations index:%d\n", config->iConfiguration);
	printf("max power:%d mA\n\n", config->MaxPower);

	int i;
	for (i = 0; i < config->bNumInterfaces; ++i) {
		log_interface(config->interface + i);
	}

	libusb_free_config_descriptor(config);
}

static void log_interface(const struct libusb_interface *intf) {
	if (NULL == intf) {
		return;
	}

	const struct libusb_interface_descriptor *desc = intf->altsetting;
	if (NULL == desc) {
		printf("Failed get interface descriptor.\n");
		return;
	}

	int i;
	for (i = 0; i < intf->num_altsetting; ++i) {
		const struct libusb_interface_descriptor *d = desc + i;
		printf("  interface No.%d\n", d->iInterface);
		printf("  interface num:%d\n", d->bInterfaceNumber);
		printf("  endpoint num:%d\n", d->bNumEndpoints);
		log_endpoint(d->endpoint, d->bNumEndpoints);
		printf("\n");
	}
}

static void log_endpoint(const struct libusb_endpoint_descriptor *desc, int cnt) {
	if (NULL == desc || cnt < 1) {
		return;
	}

	int i;
	for (i = 0; i < cnt; ++i) {
		uint8_t addr = desc[i].bEndpointAddress;
		printf("    addr:0x%x ", addr);
		if ( (addr & LIBUSB_ENDPOINT_IN) != 0) {
			printf("IN \n");
		} else {
			printf("OUT \n");
		}
	}
}
