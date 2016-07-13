#include <libmtp.h>
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>

#define EP_IN 0x81
#define EP_OUT 0x01

#define VID 0x2717
#define PID 0x0368

typedef struct {
    uint32_t containerSize;
    uint16_t containerType;
    uint16_t operationCode;
    uint32_t transactionId;
} MtpReq;

static void log_device(libusb_device_handle *dev_handle);
static void log_configuration(libusb_device_handle *dev_handle);
static void log_interface(const struct libusb_interface *intf);
static void log_endpoint(const struct libusb_endpoint_descriptor *desc, int cnt);
static void log_device_class(uint8_t cls);

libusb_device_handle *gDevHandle = NULL;

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

        gDevHandle = dev_handle;

		printf("Open device success.\n");
		log_device(dev_handle);

        libusb_reset_device(dev_handle);

        libusb_detach_kernel_driver(dev_handle, 0);

		status = libusb_claim_interface(dev_handle, 0);
		if (status < 0) {
			printf("Failed claim interface:%s\n", libusb_strerror(status));
			break;
		}

        MtpReq req;
        memset(&req, 0, sizeof(req));
        req.containerSize = 12;
		req.operationCode = 0x1001;
		int len;
		status = libusb_bulk_transfer(dev_handle,
				EP_OUT,
				(char *)&req,
				sizeof(req),
				&len,
				1000);
		if (status < 0) {
			printf("write error:%s\n", libusb_strerror(status));
			break;
		}
		printf("send %d bytes.\n", len);

#define BUFF_SIZE 1024
		char read_buf[BUFF_SIZE];
		status = libusb_bulk_transfer(dev_handle,
				EP_IN,
				read_buf,
				BUFF_SIZE,
				&len,
				1000);
		if (status < 0) {
			printf("Failed read GetDeviceInfo response:%s.\n", libusb_strerror(status));
			break;
		}
		printf("read %d bytes.\n", len);

        char *base = read_buf;

        MtpReq resp;
        memset(&resp, 0, sizeof(resp));
        memcpy(&resp, read_buf, sizeof(resp));
        printf("total length:%d\n", resp.containerSize);
        printf("type:0x%0x\n", resp.containerType);
        printf("code:0x%0x\n", resp.operationCode);
        printf("transaction id:%d\n", resp.transactionId);

        char *data = read_buf + 12;
        uint16_t ptp_ver = *(uint16_t *)data;
        data += 2;
        printf("ptp version:%d\n", ptp_ver);

        uint32_t ptp_ext = *(uint32_t *)data;
        data += 4;
        printf("ptp extension:0x%0x\n", ptp_ext);

        uint16_t mtp_ver = *(uint32_t *)data;
        data += 2;
        printf("mtp version:%d\n", mtp_ver);

        len = *(uint8_t *)data;
        data += 1;
        printf("string len:%d\n", len);

        char ext_name[128];
        memcpy(ext_name, data, len * 2);
        data += len * 2;
        printf("mtp extension name:%s\n", ext_name);

        uint16_t mode = *(uint16_t *)data;
        data += 2;
        printf("support mode:0x%0x\n", mode);

        uint32_t arr_cnt = *(uint32_t *)data;
        data += 4;
        printf("arr cnt:%d\n", arr_cnt);

        uint16_t arr_item_type = *(uint16_t *)data;
        printf("array item type:0x%0x\n", arr_item_type);


	} while (0);


    LIBMTP_Init();
    LIBMTP_raw_device_t *raw_devs = NULL;
    int cnt_raw_devs;
    LIBMTP_error_number_t err = LIBMTP_Detect_Raw_Devices(&raw_devs, &cnt_raw_devs);
    switch (err) {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
            printf("No raw devices found. \n");
            break;

        case LIBMTP_ERROR_CONNECTING:
            printf("Detect a connecting error.Exting \n");
            break;

        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            printf("Memory allocate error \n");
            break;

        case LIBMTP_ERROR_NONE:
            printf("Found %d device(s):\n", cnt_raw_devs);
            break;
    }

	if (NULL != dev_handle) {
		status = libusb_release_interface(dev_handle, 0);
		if (status < 0) {
			printf("Failed release interface:%s\n", libusb_strerror(status));
		}

		libusb_attach_kernel_driver(dev_handle, 0);
		libusb_close(dev_handle);
		dev_handle = NULL;
        gDevHandle = NULL;
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

        case 0x0210:
            printf("USB 2.1\n");
            break;

		case 0x0220:
			printf("USB 2.2\n");
			break;

		default:
			printf("Unknown version:0x%04x\n", version);
	}

    log_device_class(desc.bDeviceClass);
    log_device_class(desc.bDeviceSubClass);

    printf("device protocal:0x%x\n", desc.bDeviceProtocol);

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
		printf("  interface num:%d\n", d->bInterfaceNumber);

        log_device_class(d->bInterfaceClass);
        log_device_class(d->bInterfaceSubClass);

        char name[128];
        int len = libusb_get_string_descriptor_ascii(gDevHandle, desc[i].iInterface, name, 128);
        if (len >= 0) {
            name[len] = 0;
        } else {
            printf("  Failed get string descriptor \n");
        }
        printf("  iInterface:%s\n", name);

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

        uint8_t attr = desc[i].bmAttributes;
        int type = attr & 0x0003;
        switch (type) {
            case LIBUSB_TRANSFER_TYPE_CONTROL:
                printf("    transfer type:control \n");
                break;

            case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
                printf("    transfer type:isochronous\n");
                break;

            case LIBUSB_TRANSFER_TYPE_BULK:
                printf("    transfer type:bulk \n");
                break;

            case LIBUSB_TRANSFER_TYPE_INTERRUPT:
                printf("    transfer type:interrupt\n");
                break;

            case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
                printf("    transfer type:bulk stream \n");
                break;
        }

        printf("    maxPacketSize:%d\n", desc[i].wMaxPacketSize);
    }
}

static void log_device_class(uint8_t cls) {
    switch (cls) {
        case LIBUSB_CLASS_PER_INTERFACE:
            printf("  device per interface \n");
            break;

        case LIBUSB_CLASS_AUDIO:
            printf("  audio device \n");
            break;

        case LIBUSB_CLASS_COMM:
            printf("  communication device \n");
            break;

        case LIBUSB_CLASS_HID:
            printf("  Human interface device \n");
            break;

        case LIBUSB_CLASS_PHYSICAL:
            printf("  physical device \n");
            break;

        case LIBUSB_CLASS_PRINTER:
            printf("  printer device \n");
            break;

        case LIBUSB_CLASS_PTP:
            printf("  ptp/image device \n");
            break;

        case LIBUSB_CLASS_MASS_STORAGE:
            printf("  mass storage device \n");
            break;

        case LIBUSB_CLASS_HUB:
            printf("  hub device \n");
            break;

        case LIBUSB_CLASS_DATA:
            printf("  data device \n");
            break;

        case LIBUSB_CLASS_SMART_CARD:
            printf("  smart card device \n");
            break;

        case LIBUSB_CLASS_CONTENT_SECURITY:
            printf("  content security device \n");
            break;

        case LIBUSB_CLASS_VIDEO:
            printf("  video device \n");
            break;

        case LIBUSB_CLASS_PERSONAL_HEALTHCARE:
            printf("  personal healthcare device \n");
            break;

        case LIBUSB_CLASS_DIAGNOSTIC_DEVICE:
            printf("  diagnostic device \n");
            break;

        case LIBUSB_CLASS_WIRELESS:
            printf("  wireless device \n");
            break;

        case LIBUSB_CLASS_APPLICATION:
            printf("  application device \n");
            break;

        case LIBUSB_CLASS_VENDOR_SPEC:
            printf("  vendor specific device \n");
            break;

        default:
            printf("  Unknown device 0x%x\n", cls);
    }

}
