
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server = {0};
    struct timeval timer = {2, 0};
    socklen_t len = sizeof(server);

    char msg[100] = "Hello";
    char ack[100];

    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               &timer, sizeof(timer));

    for (int i = 1; i <= 5; i++)
    {
        printf("Attempt %d: Sending packet\n", i);

        sendto(sock, msg, strlen(msg), 0,
               (struct sockaddr *)&server, len);

        int n = recvfrom(sock, ack, sizeof(ack) - 1, 0,
                         NULL, NULL);

        if (n > 0)
        {
            ack[n] = '\0';
            printf("Received ACK: %s\n", ack);
            printf("Packet delivered successfully!\n");
            close(sock);
            return 0;
        }

        printf("Timeout! No ACK received.\n");
    }

    printf("Maximum attempts reached. Transmission failed.\n");

    close(sock);
    return 0;
}
