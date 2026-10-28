#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0};
    socklen_t len = sizeof(server);

    char roll[20], response[256];

    server.sin_family = AF_INET;
    server.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    printf("Enter roll number: ");
    fgets(roll, sizeof(roll), stdin);

    sendto(sock, roll, strlen(roll), 0,
           (struct sockaddr *)&server, len);

    int n = recvfrom(sock, response, sizeof(response) - 1, 0,
                     (struct sockaddr *)&server, &len);

    if (n > 0) {
        response[n] = '\0';
        printf("%s\n", response);
    }

    close(sock);
    return 0;
}