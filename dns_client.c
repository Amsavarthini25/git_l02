//dns client
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8085
#define SIZE 1024
#define CACHE 5

int main()
{
    int sock, count = 0;
    struct sockaddr_in server;
    struct timeval timeout = {5, 0};
    char domain[100], response[SIZE];

    char cached_name[CACHE][100];
    char cached_ip[CACHE][100];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET,"127.0.0.1",&server.sin_addr);

    while (1)
    {
        printf("\nEnter domain (or exit): ");
        scanf("%99s", domain);

        if (strcmp(domain, "exit") == 0)
            break;

        int len = strlen(domain);

        if (len < 3 || domain[0] == '.' ||
            domain[len - 1] == '.' ||
            strchr(domain, '.') == NULL ||
            strchr(domain, ' ') != NULL)
        {
            printf("ERROR: INVALID FORMAT\n");
            continue;
        }

        int found = 0;

        for (int i = 0; i < count; i++)
        {
            if (strcmp(domain, cached_name[i]) == 0)
            {
                printf("CACHE HIT: %s\n", cached_ip[i]);
                found = 1;
                break;
            }
        }

        if (found)
            continue;

        printf("CACHE MISS\n");

        sendto(sock, domain, strlen(domain), 0,
               (struct sockaddr *)&server, sizeof(server));

        memset(response, 0, sizeof(response));

        int n = recvfrom(sock, response, SIZE - 1, 0,
                         NULL, NULL);

        if (n < 0)
        {
            printf("DNS server timeout\n");
            continue;
        }

        response[n] = '\0';
        printf("Response: %s\n", response);

        if (strncmp(response, "NXDOMAIN", 8) == 0 ||
            strncmp(response, "ERROR", 5) == 0)
            continue;

        int pos;

        if (count < CACHE)
            pos = count++;
        else
        {
            for (int i = 0; i < CACHE - 1; i++)
            {
                strcpy(cached_name[i], cached_name[i + 1]);
                strcpy(cached_ip[i], cached_ip[i + 1]);
            }
            pos = CACHE - 1;
        }

        strcpy(cached_name[pos], domain);
        strcpy(cached_ip[pos], response);
    }

    close(sock);
    return 0;
}