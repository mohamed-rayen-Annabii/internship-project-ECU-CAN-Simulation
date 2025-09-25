#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <iomanip>
#include <fstream>
#include <vector>
#include <sstream>
#include <csignal>

#define DEFAULT_INTERFACE "vcan_tunnel_b"
#define REQUEST_ID 0x100
#define RESPONSE_ID 0x123
#define ACK_ID 0x101
#define NAK_ID 0x102
#define POLLING_INTERVAL_MS 1000
#define RESPONSE_TIMEOUT_MS 500
#define DELAY_THRESHOLD_MS 100

volatile sig_atomic_t running = 1;

void signal_handler(int) {
    running = 0;
}

uint8_t calculate_checksum(uint8_t seq, uint8_t speed) {
    return seq ^ speed;
}

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]";
    return oss.str();
}

void write_log(const std::vector<std::string>& message_status, int total, int received, int not_received,
               int timeouts, int corruptions, int losses, int delayed) {
    std::ofstream log_file("fault_summary_can.txt", std::ios::trunc);
    if (!log_file) {
        std::cerr << "Failed to open log file\n";
        return;
    }
    log_file << "ECU2 CAN Fault Summary\n======================\n";
    log_file << "Total messages: " << total << "\n";
    log_file << "Received: " << received << "\n";
    log_file << "Not Received: " << not_received << "\n";
    log_file << "  - Timeouts: " << timeouts << "\n";
    log_file << "  - Corruptions: " << corruptions << "\n";
    log_file << "  - Losses: " << losses << "\n";
    log_file << "  - Delayed: " << delayed << "\n\n";
    for (const auto& status : message_status) {
        log_file << status << "\n";
    }
    log_file.close();
}

int main(int argc, char* argv[]) {
    const char* interface = (argc > 1) ? argv[1] : DEFAULT_INTERFACE;

    int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RESPONSE_TIMEOUT_MS * 1000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Setsockopt failed");
        close(sockfd);
        return 1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX failed");
        close(sockfd);
        return 1;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    std::cout << "[ECU2]  Ready to poll ECU1 on " << interface << "...\n";

    uint8_t expected_seq = 0;
    int total_messages = 0, received_count = 0, not_received_count = 0;
    int timeout_count = 0, corruption_count = 0, loss_count = 0, delayed_count = 0;
    std::vector<std::string> message_status;

    signal(SIGINT, signal_handler);

    while (running) {
        struct can_frame request_frame;
        memset(&request_frame, 0, sizeof(request_frame));
        request_frame.can_id = REQUEST_ID;
        request_frame.can_dlc = 0;

        std::cout << "\n" << currentTimestamp() << " [ECU2] Sending request to ECU1 for message " << +expected_seq << "...\n";

        auto send_time = std::chrono::steady_clock::now();
        if (write(sockfd, &request_frame, sizeof(request_frame)) != sizeof(request_frame)) {
            perror("Write request failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));
            continue;
        }

        struct can_frame recv_frame;
      ssize_t nbytes = read(sockfd, &recv_frame, sizeof(recv_frame));
if (nbytes < 0) {
  //  perror("Read failed or timeout");
      std::cout << currentTimestamp() << " [ECU2] Message not received (timeout)\n";

    not_received_count++;
    timeout_count++;
    std::ostringstream log_entry;
    log_entry << "Message " << +expected_seq << ": not received ";
    message_status.push_back(log_entry.str());
    write_log(message_status, total_messages, received_count, not_received_count,
              timeout_count, corruption_count, loss_count, delayed_count);
    std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));
    continue;
}

        total_messages++;
        std::ostringstream log_entry;
        log_entry << "Message " << +expected_seq << ": ";

       if (recv_frame.can_id == RESPONSE_ID && recv_frame.can_dlc >= 3) {
            auto recv_time = std::chrono::steady_clock::now();
            double rtt = std::chrono::duration_cast<std::chrono::microseconds>(recv_time - send_time).count() / 1000.0;

            uint8_t seq = recv_frame.data[0];
            uint8_t speed = recv_frame.data[1];
            uint8_t rcvd_checksum = recv_frame.data[2];
            uint8_t calc_checksum = calculate_checksum(seq, speed);

            struct can_frame ack_nak_frame;
            memset(&ack_nak_frame, 0, sizeof(ack_nak_frame));
            ack_nak_frame.can_dlc = 1;
            ack_nak_frame.data[0] = seq;

            if (rcvd_checksum != calc_checksum) {
                std::cout << currentTimestamp() << " [ECU2]  CORRUPTION detected: seq=" << +seq << " checksum=" << +rcvd_checksum << " expected=" ;
                ack_nak_frame.can_id = NAK_ID;
                write(sockfd, &ack_nak_frame, sizeof(ack_nak_frame));
                log_entry << "not received (corruption, RTT=" << std::fixed << std::setprecision(2) << rtt << " ms)";
                not_received_count++;
                corruption_count++;
                expected_seq = seq + 1; // Sync sequence
            } else if (seq != expected_seq) {
                std::cout << currentTimestamp() << " [ECU2]  not received message expected seq=" << +expected_seq << " but got seq=" << +seq <<"\n";
                ack_nak_frame.can_id = NAK_ID;
                write(sockfd, &ack_nak_frame, sizeof(ack_nak_frame));
                log_entry << "not received (loss/mismatch, got seq=" << +seq << ", RTT=" << std::fixed << std::setprecision(2) << rtt << " ms)";
                not_received_count++;
                loss_count++;
                expected_seq = seq + 1; // Sync sequence
            } else if (rtt > DELAY_THRESHOLD_MS) {
                std::cout << currentTimestamp() << " [ECU2]  ! DELAYED message detected: seq=" << +seq << " speed=" << +speed << " RTT=" << std::fixed << std::setprecision(2) << rtt << " ms. Sending ACK.\n";
                ack_nak_frame.can_id = ACK_ID;
                write(sockfd, &ack_nak_frame, sizeof(ack_nak_frame));
                log_entry << "not received (delayed, RTT=" << std::fixed << std::setprecision(2) << rtt << " ms)";
                not_received_count++;
                delayed_count++;
                expected_seq = seq + 1;                                     
            } else {
                std::cout << currentTimestamp() << " [ECU2] Received message :  speed=" << +speed << " seq=" << +seq << " RTT=" << std::fixed << std::setprecision(2) << rtt << " ms. Sending ACK.\n";
                ack_nak_frame.can_id = ACK_ID;
                write(sockfd, &ack_nak_frame, sizeof(ack_nak_frame));
                log_entry << "received (RTT=" << std::fixed << std::setprecision(2) << rtt << " ms)";
                received_count++;
                expected_seq = seq + 1;
            }
        } else {
            log_entry << "not received (invalid frame)";
            not_received_count++;
        }

        message_status.push_back(log_entry.str());
        write_log(message_status, total_messages, received_count, not_received_count,
                 timeout_count, corruption_count, loss_count, delayed_count);

        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));
    }

    std::cout << "[ECU2] Shutting down...\n";
    close(sockfd);
    return 0;
}
