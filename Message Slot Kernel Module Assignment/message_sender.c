#include "message_slot.h"

#include <fcntl.h>	/* open */
#include <unistd.h>	/* exit */
#include <sys/ioctl.h>	/* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
	int file_desc, ret_val, channel_id;
	size_t msg_len;

	if (argc != 4) {
		perror("Wrong number of arguments");
		exit(1);
	}

	file_desc = open(argv[1], O_RDWR);
	if (file_desc < 0) {
	    perror(strerror(errno));
		exit(1);
	}

	channel_id = atoi(argv[2]);
	ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, channel_id);
	if(ret_val != SUCCESS) {
        perror(strerror(errno));
        exit(1);
	}

	msg_len = strlen(argv[3]);
	ret_val = write(file_desc, argv[3], msg_len);
	if(ret_val != msg_len) {
        perror(strerror(errno));
        exit(1);
	}

	close(file_desc);

	exit(0);
}
