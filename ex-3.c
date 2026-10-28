#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

int main()
{
    int sock;
    char host[100], path[200], request[500], buffer[4096];
    struct hostent *server;
    struct sockaddr_in addr;
    int n, total = 0;

    printf("Enter hostname: ");
    scanf("%99s", host);

    printf("Enter path (e.g. /): ");
    scanf("%199s", path);

    server = gethostbyname(host);

    if (server == NULL)
    {
        printf("Host not found\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(80); //////attentionnnnn
    memcpy(&addr.sin_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&addr,
                sizeof(addr)) < 0)
    {
        printf("Connection failed\n");
        close(sock);
        return 1;
    }

    sprintf(request,
            "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
            path, host);

    clock_t start = clock();

    send(sock, request, strlen(request), 0);

    FILE *fp = fopen("output.html", "w");

    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[n] = '\0';
        printf("%s", buffer);
        fprintf(fp, "%s", buffer);
        total += n;
    }

    fclose(fp);

    clock_t end = clock();

    double seconds = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n\nTotal received: %d bytes\n", total);
    printf("Time taken: %.3f seconds\n", seconds);

    if (seconds > 0)
        printf("Throughput: %.2f bytes/second\n",
               total / seconds);

    close(sock);
    return 0;
}