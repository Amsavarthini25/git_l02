#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080

void *handle_client(void *arg)
{
    int client = *(int *)arg;
    free(arg);

    char roll[20], response[512] = "Student Record Not Found";
    recv(client, roll, sizeof(roll) - 1, 0);
   roll[strlen(roll) - 1] = '\0';

    FILE *fp = fopen("students.csv", "r");
    char line[256];

    if (fp != NULL) {
        fgets(line, sizeof(line), fp); // Skip header

        while (fgets(line, sizeof(line), fp)) {
    char *r = strtok(line, ",");
    char *name = strtok(NULL, ",");
    char *dept = strtok(NULL, ",");
    char *sem = strtok(NULL, ",");
    char *cgpa = strtok(NULL, ",\n");

    if (r != NULL && strcmp(r, roll) == 0) {
        snprintf(response, sizeof(response),
                 "Roll: %s\nName: %s\nDepartment: %s\nSemester: %s\nCGPA: %s",
                 r, name, dept, sem, cgpa);
        break;
    }
}
        fclose(fp);
    }

    send(client, response, strlen(response) + 1, 0);
    close(client);
    return NULL;
}

int main()
{
    int server, *client;
    struct sockaddr_in addr;

    server = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 5);

    printf("TCP server listening on port %d\n", PORT);

    while (1) {
        client = malloc(sizeof(int));
        *client = accept(server, NULL, NULL);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client);
        pthread_detach(tid);
    }

    close(server);
    return 0;
}