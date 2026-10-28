
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define TOTAL 3
#define MAX_ATTEMPTS 5

double now()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1000000.0;
}

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0};
    socklen_t len = sizeof(server);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct timeval timeout = {2, 0};

    server.sin_family = AF_INET;
    server.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    char msg[300], packet[512], buffer[512];

    int transmissions = 0, retransmissions = 0;
    int ack_sent = 0, ack_received = 0;
    int replies_received = 0, failed = 0;
    int successful_bytes = 0;

    double rtt_sum = 0, min_rtt = 999999, max_rtt = 0;
    int rtt_count = 0;

    double start = now();

    for (int seq = 1; seq <= TOTAL; seq++)
    {
        printf("\nEnter message %d: ", seq);
        fflush(stdout);

        if (fgets(msg, sizeof(msg), stdin) == NULL)
            break;

        msg[strcspn(msg, "\n")] = '\0';

        snprintf(packet, sizeof(packet), "DATA|%d|%s", seq, msg);

        int delivered = 0;

        for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++)
        {
            if (attempt > 1) {
                retransmissions++;
                printf("TIMEOUT: No ACK for packet %d.\n", seq);
                printf("RETRANSMITTING packet %d (attempt %d)\n",
                       seq, attempt);
            }

            // Set 2-second timeout while waiting for ACK
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                       &timeout, sizeof(timeout));

            transmissions++;
            double sent = now();

            sendto(sock, packet, strlen(packet) + 1, 0,
                   (struct sockaddr *)&server, len);

            printf("SENT: DATA packet %d: %s\n", seq, msg);

            while (1)
            {
                int n = recvfrom(sock, buffer, sizeof(buffer) - 1,
                                 0, NULL, NULL);

                if (n < 0)
                    break;

                buffer[n] = '\0';

                int ack_seq;

                if (sscanf(buffer, "ACK|%d", &ack_seq) == 1)
                {
                    if (ack_seq != seq)
                        continue;

                    double rtt = now() - sent;
                    ack_received++;

                    printf("RECEIVED: ACK for DATA packet %d\n", seq);
                    printf("RTT: %.3f ms\n", rtt * 1000);

                    // Karn's rule: don't sample retransmitted packets
                    if (attempt == 1) {
                        rtt_sum += rtt;
                        rtt_count++;

                        if (rtt < min_rtt) min_rtt = rtt;
                        if (rtt > max_rtt) max_rtt = rtt;
                    }

                    delivered = 1;
                    break;
                }
            }

            if (delivered)
                break;
        }

        if (!delivered) {
            printf("Packet %d failed after maximum attempts.\n", seq);
            failed++;
            continue;
        }

        // Allow time for the server user to type a reply
        struct timeval reply_timeout = {30, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                   &reply_timeout, sizeof(reply_timeout));

        // Receive server reply
        while (1)
        {
            int n = recvfrom(sock, buffer, sizeof(buffer) - 1,
                             0, NULL, NULL);

            if (n < 0) {
                printf("Timed out waiting for reply %d.\n", seq);
                break;
            }

            buffer[n] = '\0';

            int reply_seq;
            char reply[400];

            if (sscanf(buffer, "REPLY|%d|%399[^\n]",
                       &reply_seq, reply) == 2)
            {
                if (reply_seq != seq)
                    continue;

                printf("RECEIVED: REPLY packet %d: %s\n",
                       seq, reply);

                replies_received++;
                successful_bytes += strlen(msg) + strlen(reply);

                snprintf(packet, sizeof(packet), "RACK|%d", seq);

                sendto(sock, packet, strlen(packet) + 1, 0,
                       (struct sockaddr *)&server, len);

                ack_sent++;

                printf("SENT: ACK for REPLY packet %d\n", seq);
                break;
            }
        }
    }

    double elapsed = now() - start;

    printf("\n========== FINAL METRICS ==========\n");
    printf("Data packets planned       : %d\n", TOTAL);
    printf("Data transmission attempts: %d\n", transmissions);
    printf("Retransmissions            : %d\n", retransmissions);
    printf("Packets not ACKed          : %d\n", failed);
    printf("Data ACKs received         : %d\n", ack_received);
    printf("Server replies received    : %d\n", replies_received);
    printf("Reply ACKs sent            : %d\n", ack_sent);

    printf("Final delivery ratio       : %.2f%%\n",
           100.0 * (TOTAL - failed) / TOTAL);

    printf("Retransmission ratio       : %.2f%%\n",
           transmissions ? 100.0 * retransmissions / transmissions : 0);

    if (rtt_count > 0) {
        printf("Average first-attempt RTT  : %.3f ms\n",
               rtt_sum * 1000 / rtt_count);
        printf("Minimum first-attempt RTT  : %.3f ms\n",
               min_rtt * 1000);
        printf("Maximum first-attempt RTT  : %.3f ms\n",
               max_rtt * 1000);
    }

    printf("Total elapsed time         : %.3f s\n", elapsed);
    printf("Application throughput    : %.2f bytes/s\n",
           elapsed > 0 ? successful_bytes / elapsed : 0);

    close(sock);
    return 0;
}
