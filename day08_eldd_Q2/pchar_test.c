#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "pchar_ioctl.h"

int main() {
    int fd;
    info_t info;

    // Open the device file
    fd = open("/dev/pchar0", O_RDWR);
    if(fd == -1) {
        perror("Failed to open device file");
        return -1;
    }

    // Get FIFO info before resizing
    if(ioctl(fd, FIFO_INFO, &info) == -1) {
        perror("Failed to get FIFO info");
        close(fd);
        return -1;
    }
    printf("Before resize: size = %d, avail = %d, len = %d\n", info.size, info.avail, info.len);

    // Resize the FIFO
    if(ioctl(fd, FIFO_RESIZE) == -1) {
        perror("Failed to resize FIFO");
        close(fd);
        return -1;
    }

    // Get FIFO info after resizing
    if(ioctl(fd, FIFO_INFO, &info) == -1) {
        perror("Failed to get FIFO info");
        close(fd);
        return -1;
    }
    printf("After resize: size = %d, avail = %d, len = %d\n", info.size, info.avail, info.len);

    // Close the device file
    close(fd);
    return 0;
}

