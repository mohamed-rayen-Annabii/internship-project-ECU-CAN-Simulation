#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <csignal>
#include <ctime>
#include <cerrno>

#define ACK_ID 0x101
#define DEFAULT_INTERFACE "vcan_tunnel_a"
#define ACK_TIMEOUT_US 200000

volatile sig_atomic_t running = 1;

// Timestamp generator to match ECU2 format
std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << "[" << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "] ";
    return oss.str();
}

// Handle Ctrl+C
void signal_handler(int) {
    running = 0;
}

int main(int argc, char* argv[]) {
    const char* interface = (argc > 1) ? argv[1] : DEFAULT_INTERFACE;

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct ifreq ifr{};
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX failed");
        close(sock);
        return 1;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return 1;
    }

    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = ACK_TIMEOUT_US;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Setsockopt failed");
        close(sock);
        return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    uint8_t seq = 0;

    std::cout << currentTimestamp() << "[ECU1] Ready, waiting for requests on " << interface << "...\n";

    signal(SIGINT, signal_handler);

    while (running) {
        struct can_frame recv_frame{};
        int nbytes = read(sock, &recv_frame, sizeof(recv_frame));
        if (nbytes < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Read error");
            }
            continue;
        }

        if (recv_frame.can_id == 0x100 && nbytes == sizeof(recv_frame)) {
          std::cout <<"\n";
            std::cout << currentTimestamp() << "[ECU1] Received request from ECU2. Preparing response...\n";

            seq++;  // wraps around automatically at 255
            uint8_t speed = dist(gen);
            uint8_t checksum = seq ^ speed;

            struct can_frame send_frame{};
            send_frame.can_id = 0x123;
            send_frame.can_dlc = 3;
            send_frame.data[0] = seq;
            send_frame.data[1] = speed;
            send_frame.data[2] = checksum;

            if (write(sock, &send_frame, sizeof(send_frame)) != sizeof(send_frame)) {
                perror("Write failed");
                continue;
            }

            std::cout << currentTimestamp()
                      << "[ECU1]  sent speed=" << +speed
                      << "   seq=" << +seq
                      << "\n" ;
                     

            struct can_frame ack_frame{};
            nbytes = read(sock, &ack_frame, sizeof(ack_frame));
            if (nbytes < 0) {
                std::cout << currentTimestamp() << "[ECU1]  No ACK received \n";
            } else if (ack_frame.can_id == ACK_ID && ack_frame.can_dlc >= 1) {
                std::cout << currentTimestamp()
                          << "[ECU1]  Received ACK " << "\n";
            }
            // NAK is ignored by design
        }
    }

    std::cout << currentTimestamp() << "[ECU1] Shutting down...\n";
    close(sock);
    return 0;
}

