#include "message_slot.h"

#include <fcntl.h>	/* open */
#include <unistd.h> /* exit */
#include <sys/ioctl.h>	/* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
	int file_desc, ret_val, channel_id;
	char buffer[BUF_LEN];

	if (argc != 3) {
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

	ret_val = read(file_desc, buffer, BUF_LEN);
	if(ret_val < 0) {
        perror(strerror(errno));
        exit(1);
	}

	close(file_desc);

	ret_val = write(STDOUT_FILENO, buffer, ret_val);
	if (ret_val == -1) {
        perror(strerror(errno));
        exit(1);
	}

	exit(0);
}
