
// crc client
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in server;
    char data[100], bits[8192], temp[8192];
    char generator[] = "10011";
    int length = 0;

    printf("Enter data: ");
    scanf("%99s", data);

    /* Convert each character to 8-bit binary */
    for (int i = 0; data[i] != '\0'; i++)
    {
        for (int b = 7; b >= 0; b--)
            bits[length++] =
                ((data[i] >> b) & 1) ? '1' : '0';
    }

    /* Append four zeros */
    for (int i = 0; i < 4; i++)
        bits[length++] = '0';

    bits[length] = '\0';
    strcpy(temp, bits);

    /* Calculate CRC remainder */
    for (int i = 0; i <= length - 5; i++)
    {
        if (temp[i] == '1')
        {
            for (int j = 0; j < 5; j++)
                temp[i + j] =
                    (temp[i + j] == generator[j]) ? '0' : '1';
        }
    }

    /* Append the four-bit remainder */
    for (int i = 0; i < 4; i++)
        bits[length - 4 + i] = temp[length - 4 + i];

    printf("CRC remainder: %s\n", temp + length - 4);
    printf("Transmitted frame: %s\n", bits);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        printf("Connection failed\n");
        close(sock);
        return 1;
    }

    /* Send length and CRC frame directly */
    send(sock, &length, sizeof(length), 0);
    send(sock, bits, length, 0);

    printf("CRC frame sent to server\n");

    close(sock);
    return 0;
}
