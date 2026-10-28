//dns server
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8085

int main()
{
    int sock;
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);
    char domain[1024], response[1024];

    char *names[] = {
        "google.com", "example.com", "yahoo.com",
        "openai.com", "localhost"
    };

    char *ips[] = {
        "142.250.0.1", "93.184.216.34",
        "98.137.11.163", "104.18.33.45",
        "127.0.0.1"
    };

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
       perror("bind");
       return 1;
   }

    printf("DNS Server running on port %d\n", PORT);

    while (1)
    {
        memset(domain, 0, sizeof(domain));

        int n = recvfrom(sock, domain, sizeof(domain) - 1, 0,
                         (struct sockaddr *)&client, &len);
        if (n <= 0)
            continue;

        domain[n] = '\0';
        printf("Query: %s\n", domain);

        if (strcmp(domain, "timeout.com") == 0)
            continue;

        if (strlen(domain) < 3 || strlen(domain) > 99 ||
            domain[0] == '.' ||
            domain[strlen(domain) - 1] == '.' ||
            strchr(domain, ' ') != NULL)
        {
            strcpy(response, "ERROR: INVALID FORMAT");
        }
        else
        {
            strcpy(response, "NXDOMAIN: DOMAIN NOT FOUND");

            for (int i = 0; i < 5; i++)
            {
                if (strcmp(domain, names[i]) == 0)
                {
                    strcpy(response, ips[i]);
                    break;
                }
            }
        }

        sendto(sock, response, strlen(response), 0,
               (struct sockaddr *)&client, len);
    }

    close(sock);
    return 0;
}


