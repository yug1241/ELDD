#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

int main() 
{
    int fd;
    fd = open("/dev/sr0", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open /dev/sr0");
        return 1;
    }

    if (ioctl(fd, CDROMEJECT, 0) < 0) 
    {
        perror("Failed to eject CD-ROM");
        close(fd);
        return 1; 
            }

    close(fd);
    return 0;
}

