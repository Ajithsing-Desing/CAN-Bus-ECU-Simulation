
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>

int main(void)
{
    int socket_fd;
    struct sockaddr_can address;
    struct ifreq interface;
    struct can_frame frame;

    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&interface, 0, sizeof(interface));
    strncpy(interface.ifr_name, "vcan0", IFNAMSIZ - 1);

    if (ioctl(socket_fd, SIOCGIFINDEX, &interface) < 0) {
        perror("CAN interface not found");
        close(socket_fd);
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.can_family = AF_CAN;
    address.can_ifindex = interface.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("CAN bind failed");
        close(socket_fd);
        return 1;
    }

    frame.can_id = 0x100;
    frame.can_dlc = 2;

    printf("Engine ECU started. Press Ctrl+C to stop.\n");

    for (int rpm = 1000; rpm <= 5000; rpm += 500) {
        frame.data[0] = (rpm >> 8) & 0xFF;
        frame.data[1] = rpm & 0xFF;

        if (write(socket_fd, &frame, sizeof(frame))
            != sizeof(frame)) {
            perror("CAN transmission failed");
            close(socket_fd);
            return 1;
        }

        printf("Transmitted Engine RPM: %d\n", rpm);
        sleep(1);
    }

    close(socket_fd);
    return 0;
}
