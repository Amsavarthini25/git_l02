#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0}, client;
    socklen_t len = sizeof(client);

    char roll[20], response[256];

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(sock, (struct sockaddr *)&server, sizeof(server));
    printf("UDP server listening on port %d\n", PORT);

    while (1) {
        int n = recvfrom(sock, roll, sizeof(roll) - 1, 0,
                         (struct sockaddr *)&client, &len);
        if (n <= 0)
            continue;

        roll[n] = '\0';
        roll[strcspn(roll, "\r\n")] = '\0';
        strcpy(response, "Student Record Not Found");

        FILE *fp = fopen("students.csv", "r");
        char line[256];

        if (fp != NULL) {
            fgets(line, sizeof(line), fp); // Skip header

            while (fgets(line, sizeof(line), fp)) {
                char r[20], name[80], dept[40];
                int sem;
                float cgpa;

                if (sscanf(line, "%19[^,],%79[^,],%39[^,],%d,%f",
                           r, name, dept, &sem, &cgpa) == 5 &&
                    strcmp(r, roll) == 0) {
                    snprintf(response, sizeof(response),
                             "Roll: %s\nName: %s", r, name);
                    break;
                }
            }
            fclose(fp);
        }

        sendto(sock, response, strlen(response) + 1, 0,
               (struct sockaddr *)&client, len);
    }

    close(sock);
    return 0;
}