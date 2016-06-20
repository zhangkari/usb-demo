#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>

#define EP_IN 0x83
#define EP_OUT 0x04

#define VID 0x2717
#define PID 0x0368

static void log_device(libusb_device *dev);

void main() {

    libusb_context *cntxt = NULL;
    libusb_device_handle *dev_handle = NULL;

    if (libusb_init(&cntxt) < 0) {
        printf("Failed init usb context.\n");
        return;
    }

    do {

        printf("good job!\n");

        libusb_set_debug(cntxt, LIBUSB_LOG_LEVEL_INFO);

        libusb_device **devs = NULL;
        int cnt = libusb_get_device_list(cntxt, &devs);
        if (cnt < 0) {
            printf("Failed get devices list.\n");
            break;
        }

        int i;
        for (i = 0; i < cnt; i++) {
            log_device(devs[i]);
        }

        printf("get devices count:%d\n", cnt);

        dev_handle = libusb_open_device_with_vid_pid(cntxt, VID, PID);
        if (NULL == dev_handle) {
            printf("Failed open device.\n");
            libusb_free_device_list(devs, 1);
            break;
        }

        printf("Open device success.\n");

        libusb_free_device_list(devs, cnt);
        if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
            printf("Kernel driver active.\n");
            if (libusb_detach_kernel_driver(dev_handle, 0) == 0) {
                printf("Kernel driver detached.\n");
            }
        }

        int status = libusb_claim_interface(dev_handle, 2);
        if (status < 0) {
            printf("Failed claim interface:%s\n", strerror(errno));
            break;
        }

        short buff = 0x1001;
        int len;
        status = libusb_bulk_transfer(dev_handle,
                EP_OUT,
                (char *)&buff,
                sizeof(buff),
                &len,
                1000);
        if (status < 0) {
            printf("Failed send GetDeviceInfo:%s.\n", strerror(errno));
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
            printf("Failed read GetDeviceInfo response:%s.\n", strerror(errno));
            break;
        }
        printf("read %d bytes.\n", len);




    } while (0);


    if (NULL != dev_handle) {
        if (libusb_release_interface(dev_handle, 0) < 0) {
            printf("Failed release interface:%s\n", strerror(errno));
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

static void log_device(libusb_device *dev) {
    if (NULL == dev) {
        return;
    }

    struct libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(dev, &desc) < 0) {
        printf("Failed get device descriptor.\n");
        return;
    }

    printf("configurations count:%d\n", desc.bNumConfigurations);
    printf("device class:%d\n", (int)desc.bDeviceClass);
    printf("vendor id:%04x\n", desc.idVendor);
    printf("product id:%04x\n", desc.idProduct);

}
