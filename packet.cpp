struct client_packet {
	int op;
	union args {
		struct connect {
			char *login;
			char *password;
		};
		struct open {
			char *path;
			int oflag;
			int mode;
		};
	}
};

struct server_packet {
	int res;
	union ret {
		struct connect {};
		struct open {
			int fd;
		};
	}
};

int read_to_end(int sock, char *buf, int size) {
	//TODO
}

int read_string(int sock, client_packet *packet) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4)) return -1;
	size = ntohs(size);
	//TODO
	return -1;
}

int read_client_packet(int sock, client_packet *packet) {
	uint32_t size;
	if (read_to_end(sock, &size, 4)) return -1;
	size = ntohs(size);

	char *buf = malloc(size);
	read_to_end(sock, &buf, size);
	//TODO
	return -1;
}