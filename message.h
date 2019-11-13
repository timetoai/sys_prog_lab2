enum action_flags {ADD, DELETE, CHECK};

struct mac_addr{
	unsigned short oct[3];
};

struct message {
	struct mac_addr mac;
	unsigned flag:8;
};