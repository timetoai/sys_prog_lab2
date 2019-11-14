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
#include <time.h>
#include "message.h"

typedef struct s_list
{
  struct mac_addr mac;
  struct s_list *prev;
  struct s_list *next;
} list;

struct message_with_addr
{
	struct message *msg;
	struct sockaddr *client_addr;
	socklen_t *client_addr_len;
};

list *mlb, *mle;
static const char *optString = "wdl:a:p:vh";
int l2wait = 0, l2port, msg_count = 0, sock;
char *l2logfile;
uint32_t l2addr = 0;
struct sockaddr_in addr;
bool init_port = false, init_wait = false, init_addr = false, init_log = false;
FILE *fl, *fm;
time_t t0 = time(NULL);

void atexit_handler();
void display_usage();
void exit_signal_handler(int signum);
void log_signal_handler(int signum);
void *message_handler(void *argv);
list* check_mac(struct mac_addr *mac);
int add_mac(struct mac_addr *mac);
int delete_mac(struct mac_addr *mac);
void server();
uint32_t *getaddr(char *temp);

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
	bool daemon = false;
	int opt = getopt(argc, argv, optString);
	char *temp_addr, *temp_log;
	while(opt != -1)
	{
		switch(opt)
		{
			case 'w':
				init_wait = true;
				l2wait = strtol(argv[optind], NULL, 10);
				if (l2wait == 0 && *argv[optind] != '0') 
				{ printf("Wrong usage of -w\n"); display_usage(); }
				break;
			case 'd':
				daemon = true;
				break;
			case 'l':
				init_log = true;
				temp_log = argv[optind - 1];
				break;
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
				printf("lab2server version 1.0");
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

	if (init_log)
	{
		envptr = NULL;
		if ((fl = fopen(l2logfile, "a")) == NULL)
		{
			if (envptr = getenv("L2LOGFILE")) { fl = fopen(envptr, "a"); }
		}
		if (fl == NULL) { fl = fopen("/tmp/lab2.log", "a"); }
	} else { fl = fopen("/tmp/lab2.log", "a"); }
	if (fl == NULL) { printf("lab2server: Error: Can't access log file\n"); exit(1); }

	if (daemon) 
	{
        int pid = fork();
        if (pid < 0) { printf("lab2server: Error: Start daemon failed\n"); exit(1); }
        if (pid == 0) { server(); }
        if (pid > 0) { printf("lab2server: Daemon successfully started\n"); exit(0); }
	}
	else { server(); }
}

void server()
{
	//Подготовка сокета к работе
	printf("lab2server: Starting server on %d.%d.%d.%d:%d\n", l2addr >> 24, l2addr << 8 >> 24, \
		l2addr << 16 >> 24, l2addr << 24 >> 24, l2port);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        printf("lab2server: Error creating socket\n");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(l2port);
    addr.sin_addr.s_addr = htonl(l2addr);
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("lab2server: Error binding socket");
        exit(1);
    }
    //Считывание сообщений
    int bytes_read;
    while(1)
    {
    	pthread_t thread;
    	struct sockaddr *client_addr = (struct sockaddr*)malloc(24);
    	socklen_t *client_addr_len = (socklen_t*)malloc(1);
    	*client_addr_len = sizeof(client_addr);
		struct message *msg = (struct message*) malloc(7);

        recvfrom(sock, (void *) msg, 7, 0, client_addr, client_addr_len);
        struct message_with_addr *mwa = (struct message_with_addr*)malloc(sizeof(struct message_with_addr));
        mwa->msg = msg;
        mwa->client_addr = client_addr;
        mwa->client_addr_len = client_addr_len;
        if (l2wait) { sleep(l2wait); }
        TEMP_FAILURE_RETRY(pthread_create(&thread, NULL, message_handler, (void*)mwa));
        msg_count++;
    }
}

void *message_handler(void *argv)
{
	struct message_with_addr *mwa = (struct message_with_addr*)argv;
	struct message *msg = mwa->msg;
	struct sockaddr *client_addr = mwa->client_addr;
	socklen_t *client_addr_len = mwa->client_addr_len;
	char ans;
	switch (msg->flag)
	{
		case 0: //ADD
			printf("lab2server: Received MAC: %x:%x:%x flag=%d\n", msg->mac.oct[0], msg->mac.oct[1], msg->mac.oct[2], msg->flag);
			ans = add_mac(&msg->mac); //0 - mac добавлен, 1 - mac уже существует
			break;
		case 1: //DELETE
			printf("lab2server: Received MAC: %x:%x:%x flag=%d\n", msg->mac.oct[0], msg->mac.oct[1], msg->mac.oct[2], msg->flag);
			ans = delete_mac(&msg->mac); //0 - mac удалено, 1 - mac'а нет в списке
			break;
		case 2: //CHECK
			printf("lab2server: Received MAC: %x:%x:%x flag=%d\n", msg->mac.oct[0], msg->mac.oct[1], msg->mac.oct[2], msg->flag);
			if(check_mac(&msg->mac)) { ans = 1; } else { ans = 0; } // 1 - mac в списке, 0 - mac не в списке
			break;
		default:
      		printf("lab2server: Got incorrect flag '%d'\n", msg->flag);
      		ans = 2;
			break;
	}
    free(msg);
    printf("lab2server: Answer is %d\n", ans);
    sendto(sock, (void*) &ans, sizeof(ans), 0, client_addr, *client_addr_len);
    free(client_addr);
    free(client_addr_len);
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
			if (found->prev) { found->prev->next = found->next; }
			else if (found->next) { mlb = found->next; } else { mlb = NULL; }
			if (found->next) { found->next->prev = found->prev; }
			else if (found->prev) { mle = found->prev; } else { mle = NULL; }
			free(found);
			return 0;
		}
		else { return 1; }
	}
	else { return 1; }
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

void display_usage()
{
	printf("Displaying usage\n");
	exit(0);
}

void atexit_handler()
{
	list* tl;
	while(mlb) { tl = mlb; mlb = mlb->next; free(tl); }
	if (fl != NULL) { fclose(fl); }
	// time_t t2 = time(NULL);
	// struct tm *t2m = localtime(&t2);
	// printf("%d.%d.%d %d:%d:%d\n", t2m->tm_mday, t2m->tm_mon, t2m->tm_year, \
	// 	t2m->tm_hour, t2m->tm_min, t2m->tm_sec); //ДД.ММ.ГГ чч:мм:сс
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