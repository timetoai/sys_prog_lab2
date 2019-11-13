#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <cerrno>
#include "message.h"

typedef struct s_list
{
  struct mac_addr mac;
  struct s_list *prev;
  struct s_list *next;
} list;

list *mlb, *mle;
static const char *optString = "wdl:a:p:vh";
int l2wait = 0, l2port = 3425, msg_count = 0;
char *l2logfile = NULL, *l2addr = NULL;

int sock;
struct sockaddr_in addr;

void atexit_handler();
void display_usage();
void exit_signal_handler(int signum);
void log_signal_handler(int signum);
void *message_handler(void *argv);
list* check_mac(struct mac_addr *mac);
int add_mac(struct mac_addr *mac);
int delete_mac(struct mac_addr *mac);

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
	sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(l2port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }
    //Считывание сообщений
    int bytes_read;
    while(1)
    {
    	pthread_t thread;
		struct message *msg = (struct message*) malloc(7);
        bytes_read = recvfrom(sock, (void *) msg, 7, 0, NULL, NULL);
        TEMP_FAILURE_RETRY(pthread_create(&thread, NULL, message_handler, (void*)msg));
    }

    return 0;
}

void *message_handler(void *argv)
{
	struct message *msg = (struct message*)argv;
	char ans;
	switch (msg->flag)
	{
		case 0: //ADD
			printf("lab2serverReceived MAC: %x:%x:%x flag=%d\n", msg->mac.oct[0], msg->mac.oct[1], msg->mac.oct[2], msg->flag);
			ans = add_mac(&msg->mac); //0 - mac добавлен, 1 - mac уже существует
			break;
		case 1: //DELETE
			printf("lab2serverReceived MAC: %x:%x:%x flag=%d\n", msg->mac.oct[0], msg->mac.oct[1], msg->mac.oct[2], msg->flag);
			ans = delete_mac(&msg->mac); //0 - mac удалено, 1 - mac'а нет в списке
			break;
		case 2: //CHECK
			printf("lab2serverReceived MAC: %x:%x:%x flag=%d\n", msg->mac.oct[0], msg->mac.oct[1], msg->mac.oct[2], msg->flag);
			if(check_mac(&msg->mac)) { ans = 1; } else { ans = 0; } // 1 - mac в списке, 0 - mac не в списке
			break;
		default:
      		printf("lab2server: Got incorrect flag '%d'\n", msg->flag);
      		ans = 2;
			break;
	}
    free(msg);
    printf("lab2server: Answer is %d\n", ans);
    //sendto(sock, (void*) &ans, sizeof(ans), 0, (struct sockaddr *)&addr, sizeof(addr));
    return 0;
}

list* check_mac(struct mac_addr *mac)
{
	if (mlb)
	{
		list* m = mlb;
		while(m->mac.oct[0] != mac->oct[0] || m->mac.oct[1] != mac->oct[1] || m->mac.oct[2] != mac->oct[2])
		{
			if (m->next) { m = m->next; }
			else { return NULL; }
		}
		return m;
	}
	else { return NULL;	}
}

int add_mac(struct mac_addr *mac)
{
	if (mlb)
	{
		if (check_mac(mac)) { return 1; }
		else 
		{
			mle->next = (list*)malloc(sizeof(list));
			mle = mle->next;
			memcpy((void*)&mle->mac, (void*)mac, sizeof(mac));
			return 0;
		}
	}
	else
	{
		mlb = (list*)malloc(sizeof(list));
		mle = mlb;
		memcpy((void*)&mle->mac, (void*)mac, sizeof(mac));
		return 0;
	}
}

int delete_mac(struct mac_addr *mac)
{
	if (mlb)
	{
		if (list *found = check_mac(mac))
		{
			found->prev->next = found->next;
			found->next->prev = found->prev;
			free(found);
			return 0;
		}
		else { return 1; }
	}
	else { return 1; }
}

void display_usage()
{
	printf("Displaying usage\n");
	exit(0);
}

void atexit_handler()
{
	if (mlb) { while(mlb) { mlb = mlb->next; free(mlb->prev); }	}
}

void exit_signal_handler(int signum)
{
	printf("lab2server: Exit by signal with number %d\n", signum);
	exit(0);
}

void log_signal_handler(int signum)
{
	printf("lab2server: logging...\n");
}