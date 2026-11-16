
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define ENGINE_CAN_ID 0x100
#define BRAKE_CAN_ID  0x200
#define TIMEOUT_SECONDS 3.0

// Return elapsed monotonic time in seconds
static double monotonic_seconds(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("Clock error");
        return -1.0;
    }

    return (double)ts.tv_sec +
           (double)ts.tv_nsec / 1000000000.0;
}

int main(void)
{
    int socket_fd;
    struct sockaddr_can address;
    struct ifreq interface;
    struct can_frame frame;
    int communication_lost = 0;

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

    // Accept Engine ECU and Brake ECU messages
    struct can_filter filters[2] = {
        {
            .can_id = ENGINE_CAN_ID,
            .can_mask = CAN_SFF_MASK |
                        CAN_EFF_FLAG |
                        CAN_RTR_FLAG
        },
        {
            .can_id = BRAKE_CAN_ID,
            .can_mask = CAN_SFF_MASK |
                        CAN_EFF_FLAG |
                        CAN_RTR_FLAG
        }
    };

    if (setsockopt(socket_fd, SOL_CAN_RAW,
                   CAN_RAW_FILTER,
                   filters, sizeof(filters)) < 0) {
        perror("CAN filter failed");
        close(socket_fd);
        return 1;
    }

    printf("Dashboard ECU started. Monitoring CAN bus...\n");
    fflush(stdout);

    // Track the last valid Engine ECU message
    double last_valid_engine_time = monotonic_seconds();

    if (last_valid_engine_time < 0) {
        close(socket_fd);
        return 1;
    }

    while (1) {
        fd_set read_fds;
        struct timeval timeout;

        double now = monotonic_seconds();

        if (now < 0) {
            break;
        }

        double elapsed = now - last_valid_engine_time;

        // Detect missing valid Engine RPM messages
        if (elapsed >= TIMEOUT_SECONDS && !communication_lost) {
            printf("WARNING: Engine ECU communication timeout!\n");
            fflush(stdout);
            communication_lost = 1;
        }

        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);

        // Check for messages every 100 milliseconds
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

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

        if (result == 0) {
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

        if (bytes != sizeof(frame)) {
            printf("Invalid CAN frame received\n");
            fflush(stdout);
            continue;
        }

        // Process Engine ECU messages
        if (frame.can_id == ENGINE_CAN_ID) {

            if (frame.can_dlc != 2) {
                printf("Invalid Engine ECU frame\n");
                fflush(stdout);
                continue;
            }

            int rpm = (frame.data[0] << 8) | frame.data[1];

            // Reject RPM values outside simulated range
            if (rpm > 8000) {
                printf("WARNING: Invalid engine RPM value!\n");
                fflush(stdout);
                continue;
            }

            last_valid_engine_time = monotonic_seconds();

            if (last_valid_engine_time < 0) {
                break;
            }

            if (communication_lost) {
                printf("Engine ECU communication restored!\n");
                communication_lost = 0;
            }

            printf("Dashboard: Engine RPM = %d\n", rpm);

            if (rpm > 4000) {
                printf("WARNING: High engine RPM!\n");
            }
        }

        // Process Brake ECU messages
        else if (frame.can_id == BRAKE_CAN_ID) {

            if (frame.can_dlc != 1) {
                printf("Invalid Brake ECU frame\n");
                fflush(stdout);
                continue;
            }

            if (frame.data[0] > 1) {
                printf("WARNING: Invalid brake status!\n");
                fflush(stdout);
                continue;
            }

            printf("Dashboard: Brake %s\n",
                   frame.data[0] ? "PRESSED" : "RELEASED");
        }

        fflush(stdout);
    }

    close(socket_fd);
    return 0;
}
