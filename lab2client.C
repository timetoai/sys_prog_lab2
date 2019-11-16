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
static const char *optString = "a:p:vh";
int l2port, sock;
uint32_t l2addr = 0;

uint32_t *getaddr(char *temp);
void atexit_handler();
void display_usage();

int main(int argc, char** argv)
{
    //Обработка завершения программы
    atexit(atexit_handler);

    //Обработка ключей
    int opt = getopt(argc, argv, optString);
    char *temp_addr;
    bool init_addr = false, init_port = false;
    while(opt != -1)
    {
        switch(opt)
        {
            case 'a':
                init_addr = true;
                temp_addr = argv[optind - 1];
                break;
            case 'p':
                init_port = true;
                l2port = strtol(argv[optind - 1], NULL, 10);
                if (l2port == 0 && *argv[optind - 1] != '0') 
                { printf("Wrong usage of -p\n"); display_usage(); }
                break;
            case 'v':
                printf("lab2client version 1.0\n");
                exit(0);
            case 'h':
            default:
                display_usage();
        }
        opt = getopt(argc, argv, optString);
    }

    // Установка адреса сервера
    char *envptr;
    if (init_addr)
    {
        uint32_t *u = getaddr(temp_addr);
        if (u) { l2addr = *u; free(u); }
        else { printf("Wrong usage of -a\n"); display_usage(); }
    } else if (envptr = getenv("L2ADDR")) 
    {
        uint32_t *u = getaddr(temp_addr);
        if (u) { l2addr = *u; free(u); }
        else { l2addr = INADDR_LOOPBACK; }
    } else { l2addr = INADDR_LOOPBACK; }

    if (!init_port)
    {
        envptr = NULL;
        if (envptr = getenv("L2PORT")) 
        {
            if (l2port = strtol(envptr, NULL, 10) == 0) 
            {
                if (*envptr == '0') { l2port = 0;}
                else { l2port = 3425; }
            }
        } else { l2port = 3425; }
    }

    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(l2port);
    addr.sin_addr.s_addr = htonl(l2addr);
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

// Парсинг адреса
uint32_t *getaddr(char *temp)
{
    uint32_t *a = (uint32_t*)malloc(4);
    int add;
    for (int i = 3; i > 0; i--)
    {
        if (*temp == '0' && *(temp + 1) == '.')  { temp += 2; }
        else if (add = strtol(temp, &temp, 10)) 
            { *a += add << 8 * i;  if (*temp != '.') { free(a); return NULL; } temp++; }
            else { free(a); return NULL; }
    }
    if (add = strtol(temp, &temp, 10)) { *a += add; }
    else if (*temp != '0') { free(a); return NULL; }
    return a;
}

void atexit_handler()
{
    close(sock);
    free(mac);
    free(f);
    free(msg);
}

void display_usage()
{
    printf("Usage: lab2client [Options]\n\
Program sends mac addresses with action flag to server\n\n\
Options:\n\
-a I\t I - ip addres, that server is listening (127.0.0.1 by default)\n\
-p P\t P - number of port, that server is listening (3425 by default)\n\
-v  \t Display program version\n\
-h  \t Display usage\n\n\
If I,P isn't set, program checks environment variables L2ADDR, L2PORT\n");
    exit(0);
}