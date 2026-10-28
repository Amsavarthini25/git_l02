#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server = {0};
    char roll[20], response[512];

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    printf("Enter roll number: ");
    fgets(roll, sizeof(roll), stdin);

    send(sock, roll, strlen(roll) + 1, 0);
    int n = recv(sock, response, sizeof(response) - 1, 0);

    if (n > 0) {
        response[n] = '\0';
        printf("%s\n", response);
    }

    close(sock);
    return 0;
}