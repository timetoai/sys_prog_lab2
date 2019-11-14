#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "message.h"

struct message *msg;
char *f, *mac, *ans;
int sock;
void atexit_handler();

int main()
{
    //Обработка завершения программы
    atexit(atexit_handler);

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
    ans = (char*) malloc(1);

    while(1)
    {
        printf("Enter mac address in format ****:****:****\n");
        scanf("%s", mac);
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
        if (!valid) {printf("Incorrect mac address\n"); continue;}

        printf("Enter flag (ADD 0, DELETE 1, CHECK 2): ");
        scanf("%s", f);
        if (!(f[0] >= '0' && f[0] <= '2')) {printf("Incorrect flag\n"); continue;}

        msg->mac.oct[0] = strtol(mac, NULL, 16);
        msg->mac.oct[1] = strtol(mac + 5, NULL, 16);
        msg->mac.oct[2] = strtol(mac + 10, NULL, 16);
        msg->flag = atoi(f);
        printf("lab2client: Sending...\n");

        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        send(sock, (void*) msg, sizeof(msg), 0);
        recv(sock, (void *) ans, 1, 0);
        printf("lab2client: Got answer with code %d\n\n", *ans);
    }

    close(sock);

    return 0;
}

void atexit_handler()
{
    close(sock);
    free(mac);
    free(f);
    free(msg);
}
