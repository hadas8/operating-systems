#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include<linux/ioctl.h>
#define MAJOR_NUM 240
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "message_slot_device"
#define BUF_LEN 128
#define SUCCESS 0

#endif
