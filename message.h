enum flags {ADD, DELETE, CHECK};

struct message {
	unsigned short mac_addr[3];
	unsigned flag:8;
};