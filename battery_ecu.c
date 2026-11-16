
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

    // Create CAN socket
    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Select virtual CAN interface
    memset(&interface, 0, sizeof(interface));
    strncpy(interface.ifr_name, "vcan0", IFNAMSIZ - 1);

    if (ioctl(socket_fd, SIOCGIFINDEX, &interface) < 0) {
        perror("CAN interface not found");
        close(socket_fd);
        return 1;
    }

    // Bind socket to vcan0
    memset(&address, 0, sizeof(address));
    address.can_family = AF_CAN;
    address.can_ifindex = interface.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("CAN bind failed");
        close(socket_fd);
        return 1;
    }

    // Battery voltage values in millivolts
    int voltages[] = {
        12600, 12400, 12100, 11900,
        11700, 11500, 12000, 12800
    };

    int count = sizeof(voltages) / sizeof(voltages[0]);

    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x300;
    frame.can_dlc = 2;

    printf("Battery ECU started. Press Ctrl+C to stop.\n");

    int index = 0;

    while (1) {
        int millivolts = voltages[index];

        // Encode voltage into two CAN data bytes
        frame.data[0] = (millivolts >> 8) & 0xFF;
        frame.data[1] = millivolts & 0xFF;

        ssize_t bytes = write(socket_fd, &frame, sizeof(frame));

        if (bytes != sizeof(frame)) {
            perror("CAN transmission failed");
            break;
        }

        printf("Battery ECU: Voltage = %.2f V\n",
               millivolts / 1000.0);
        fflush(stdout);

        index = (index + 1) % count;
        sleep(1);
    }

    close(socket_fd);
    return 0;
}

