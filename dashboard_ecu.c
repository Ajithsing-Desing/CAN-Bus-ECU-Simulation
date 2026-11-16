
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

int main(void)
{
    int socket_fd;
    struct sockaddr_can address;
    struct ifreq interface;
    struct can_frame frame;
    int communication_lost = 0;

    // Create a CAN socket
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

    // Receive only standard data frames with ID 0x100
    struct can_filter filter = {
        .can_id = 0x100,
        .can_mask = CAN_SFF_MASK |
                    CAN_EFF_FLAG |
                    CAN_RTR_FLAG
    };

    if (setsockopt(socket_fd, SOL_CAN_RAW,
                   CAN_RAW_FILTER,
                   &filter, sizeof(filter)) < 0) {
        perror("CAN filter failed");
        close(socket_fd);
        return 1;
    }

    printf("Dashboard ECU started. Waiting for RPM...\n");
    fflush(stdout);

    while (1) {
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);

        // Wait up to 3 seconds for a CAN message
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int result = select(socket_fd + 1,
                            &read_fds,
                            NULL,
                            NULL,
                            &timeout);

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("CAN select failed");
            break;
        }

        // No CAN message received within 3 seconds
        if (result == 0) {
            if (!communication_lost) {
                printf("WARNING: CAN communication timeout!\n");
                fflush(stdout);
                communication_lost = 1;
            }
            continue;
        }

        ssize_t bytes = read(socket_fd, &frame, sizeof(frame));

        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("CAN receive failed");
            break;
        }

        if (bytes != sizeof(frame) || frame.can_dlc != 2) {
            printf("Invalid CAN frame received\n");
            fflush(stdout);
            continue;
        }

        // Valid RPM message received after timeout
        if (communication_lost) {
            printf("CAN communication restored!\n");
            communication_lost = 0;
        }

        // Decode engine RPM
        int rpm = (frame.data[0] << 8) | frame.data[1];

        printf("Dashboard: Engine RPM = %d\n", rpm);

        // High RPM warning
        if (rpm > 4000) {
            printf("WARNING: High engine RPM!\n");
        }

        fflush(stdout);
    }

    close(socket_fd);
    return 0;
}
