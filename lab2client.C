#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "message.h"

struct message *msg;
char *f, *mac;
void atexit_handler();

int main()
{
    //Обработка завершения программы
    atexit(atexit_handler);

    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    msg = (struct message*) malloc(7);
    mac = (char*) malloc(15);
    f = (char*) malloc(1);

    while(1)
    {
        fputs("Enter mac address in format ****:****:****\n", stdout);
        fgets(mac, 15, stdin);
        bool valid = true;
        for (int i = 0; i < 11 ; i+=5)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (!isxdigit(mac[i + j])) 
                {
                    valid = false;
                    break;
                }
            }
            if (!valid) {break;}
        }
        if (!valid) {fputs("Incorrect mac address\n", stdout); continue;}

        fputs("Enter flag (ADD 0, DELETE 1, CHECK 2): ", stdout);
        fgets(f, 1, stdin);
        if (!(isdigit(*f) || atoi(f) < 2)) {fputs("Incorrect flag\n", stdout); continue;}

        msg->mac_addr[0] = strtol(mac, NULL, 16);
        msg->mac_addr[1] = strtol(mac + 5, NULL, 16);
        msg->mac_addr[2] = strtol(mac + 10, NULL, 16);
        msg->flag = atoi(f);
        printf("Sending...\n");
        sendto(sock, (void*) msg, sizeof(msg), 0, (struct sockaddr *)&addr, sizeof(addr));
    }

    close(sock);

    return 0;
}

void atexit_handler()
{
    free(mac);
    free(f);
    free(msg);
}
