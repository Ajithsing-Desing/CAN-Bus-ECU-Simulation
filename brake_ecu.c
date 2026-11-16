
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

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

    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x200;
    frame.can_dlc = 1;

    printf("Brake ECU started. Press Ctrl+C to stop.\n");

    int brake_pressed = 0;

    while (1) {
        frame.data[0] = brake_pressed;

        ssize_t bytes = write(socket_fd, &frame, sizeof(frame));

        if (bytes != sizeof(frame)) {
            perror("CAN transmission failed");
            break;
        }

        printf("Brake ECU: Brake %s\n",
               brake_pressed ? "PRESSED" : "RELEASED");
        fflush(stdout);

        brake_pressed = !brake_pressed;
        sleep(1);
    }

    close(socket_fd);
    return 0;
}
