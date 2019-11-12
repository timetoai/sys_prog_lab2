#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <cstring>
#include <unistd.h>
#include "message.h"

struct message **msgs;
static const char *optString = "wdl:a:p:vh";
int l2wait = 0;
char *l2logfile = NULL;
char *l2addr = NULL;
int l2port = 3425;

void atexit_handler();
void display_usage();
void exit_signal_handler(int signum);
void log_signal_handler(int signum);

int main(int argc, char **argv)
{
	//Обработка завершения программы
	atexit(atexit_handler);

    //Обработка сигналов
    struct sigaction act;
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM); 
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGUSR1);

    memset(&act, 0, sizeof(act));
	act.sa_handler = exit_signal_handler;
	act.sa_mask = sigset;
	sigaction(SIGINT, &act, 0);
	sigaction(SIGTERM, &act, 0);
	sigaction(SIGQUIT, &act, 0);

	memset(&act, 0, sizeof(act));
	act.sa_handler = log_signal_handler;
	act.sa_mask = sigset;
	sigaction(SIGUSR1, &act, 0);

	sigprocmask(SIG_UNBLOCK, &sigset, NULL);

	//Обработка ключей
	int opt = getopt(argc, argv, optString);
	while(opt != -1)
	{
		switch(opt)
		{
			case 'w':
				break;
			case 'd':
				break;
			case 'l':
				break;
			case 'a':
				break;
			case 'p':
				break;
			case 'v':
				printf("lab2server version 1.0");
				exit(0);
			case 'h':
			default:
				display_usage();
		}
		opt = getopt(argc, argv, optString);
	}

	//Подготовка сокета к работе
	int sock;
	struct sockaddr_in addr;
	struct message *msg = (struct message*) malloc(7);
	int bytes_read;
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    //Считывание сообщений
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    while(1)
    {
        bytes_read = recvfrom(sock, (void *) msg, 7, 0, NULL, NULL);
        if (msg->flag > 2)
        {
        	printf("Got incorrect flag '%d'", msg->flag);
        	continue;
        }
        printf("Received MAC: %x:%x:%x flag=%d\n", msg->mac_addr[0], msg->mac_addr[1], msg->mac_addr[2], msg->flag);
    }

    return 0;
}

void display_usage()
{
	printf("Displaying usage\n");
	exit(0);
}

void atexit_handler()
{
	
}

void exit_signal_handler(int signum)
{
	printf("Exit by signal with number %d\n", signum);
	exit(0);
}

void log_signal_handler(int signum)
{
	printf("logging...\n");
}