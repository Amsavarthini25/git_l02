
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0}, client;
    socklen_t len = sizeof(client);

    char buffer[512], msg[300], reply[300], packet[512];
    int dropped2 = 0;

    server.sin_family = AF_INET;
    server.sin_port = htons(8081);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&server, sizeof(server));

    printf("Server running on port 8081\n");
    fflush(stdout);

    while (1)
    {
        len = sizeof(client);

        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client, &len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        int seq;

        // Receive ACK for server reply
        if (sscanf(buffer, "RACK|%d", &seq) == 1) {
            printf("RECEIVED: ACK for server reply %d\n", seq);
            continue;
        }

        // Receive client data packet
        if (sscanf(buffer, "DATA|%d|%299[^\n]", &seq, msg) != 2)
            continue;

        printf("\nRECEIVED: DATA packet %d: %s\n", seq, msg);

        // Drop only the first attempt of packet 2
        if (seq == 2 && dropped2 == 0) {
            dropped2 = 1;
            printf("SIMULATED LOSS: DATA packet 2 dropped!\n");
            printf("No ACK sent for packet 2.\n");
            fflush(stdout);
            continue;
        }

        // Send ACK for received data packet
        snprintf(packet, sizeof(packet), "ACK|%d", seq);

        sendto(sock, packet, strlen(packet) + 1, 0,
               (struct sockaddr *)&client, len);

        printf("SENT: ACK for DATA packet %d\n", seq);

        // Get reply from server user
        printf("Enter reply for packet %d: ", seq);
        fflush(stdout);

        if (fgets(reply, sizeof(reply), stdin) == NULL)
            strcpy(reply, "No reply");

        reply[strcspn(reply, "\n")] = '\0';

        // Send reply to client
        snprintf(packet, sizeof(packet), "REPLY|%d|%s", seq, reply);

        sendto(sock, packet, strlen(packet) + 1, 0,
               (struct sockaddr *)&client, len);

        printf("SENT: REPLY packet %d: %s\n", seq, reply);
        fflush(stdout);
    }

    close(sock);
    return 0;
}
