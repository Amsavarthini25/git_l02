
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server = {0}, client;
    socklen_t len = sizeof(client);

    char msg[100], ack[] = "ACK";
    char choice;
    int dropped = 0;

    printf("Simulate packet loss? (y/n): ");
    scanf(" %c", &choice);

    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&server, sizeof(server));

    while (1)
    {
        recvfrom(sock, msg, sizeof(msg) - 1, 0,
                 (struct sockaddr *)&client, &len);

        msg[sizeof(msg) - 1] = '\0';

        if (choice == 'y' && dropped == 0)
        {
            printf("Packet lost intentionally!\n");
            dropped = 1;
            continue;
        }

        printf("Received: %s\n", msg);

        sendto(sock, ack, strlen(ack), 0,
               (struct sockaddr *)&client, len);

        break;
    }

    close(sock);
    return 0;
}
