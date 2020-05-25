#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

enum op_code {
	CONNECT = 1,
	OPEN = 2,
	CLOSE = 3,
	READ = 4,
	WRITE = 5,
	LSEEK = 6,
	UNLINK = 7,
	OPENDIR = 8,
	READDIR = 9,
	CLOSEDIR = 10,
	FSTAT = 11,
	STAT = 12,
};

struct packet_stat {
    char *name;
    int mode;
    int uid;
    int size;
};

struct client_packet {
	int op;
	union {
		struct {
			char *login;
			char *password;
		} connect;
		struct {
			char *path;
			int oflag;
			int mode;
		} open;
		struct {
			int fd;
		} close;
		struct {
			int fd;
			int size;
		} read;
		struct {
			int fd;
			void *data;
			int size;
		} write;
		struct {
			int fd;
			int offset;
			int whence;
		} lseek;
		struct {
			char *path;
		} unlink;
		struct {
			char *path;
		} opendir;
		struct {
			int dir_fd;
		} readdir;
		struct {
			int dir_fd;
		} closedir;
		struct {
			int fd;
		} fstat;
		struct {
			char *path;
		} stat;
	} args;
};

struct server_packet {
	int res;
	union ret {
		struct {} connect;
		struct {
			int fd;
		} open;
		struct {} close;
		struct {
			void *data;
			int size;
		} read;
		struct {} write;
		struct {
			int offset;
		} lseek;
		struct {} unlink;
		struct {
			int fd;
		} opendir;
		struct {
			char *name;
		} readdir;
		struct {} closedir;
		struct {
			packet_stat stat;
		} fstat;
		struct {
			packet_stat stat;
		} stat;
	};
};

int read_to_end(int sock, char *buf, int size) {
	int left = size;
	while (left > 0) {
		int n = read(sock, buf, left);
		if (n < 1) return -1;
		left -= n;
		buf += n;
	}
	return 0;
}

int write_all(int sock, const char *buf, int size) {
	int left = size;
	while (left > 0) {
		int n = write(sock, buf, left);
		if (n < 1) return -1;
		left -= n;
		buf += n;
	}
	return 0;
}

int read_string(int sock, char **str) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4) == -1) return -1;
	size = ntohl(size);
	cout << "Reading " << size << " bytes" << endl;
	*str = (char *) malloc(size + 1);
	if (read_to_end(sock, *str, size) == -1) return -1;
	(*str)[size] = 0;
	return 0;
}

int read_client_packet(int sock, client_packet *packet) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4) == -1) return -1;
	size = ntohl(size);

	cout << "Size: " << size << endl;
	if (size > 128) return -1;

	// char *buf = (char *) malloc(size);
	// read_to_end(sock, buf, size);
	// cout << "[" << hex;
	// for (unsigned int i = 0; i < size; i++) {
	// 	if (i != 0) cout << ", ";
	// 	cout << "0x" << (int) buf[i];
	// }
	// cout << dec << "]" << endl;
	// free(buf);

	if (read_to_end(sock, (char *) &packet->op, 1) == -1) {
		cout << "Reading op error" << endl;
		return -1;
	}
	switch (packet->op) {
		case CONNECT:
			if (read_string(sock, &packet->args.connect.login) == -1) {
				cout << "Reading login error" << endl;
				return -1;
			}
			if (read_string(sock, &packet->args.connect.password) == -1) {
				cout << "Reading password error" << endl;
				return -1;
			}
			break;
		default:
			cout << "Unknown op: " << packet->op << endl;
			return -1;
	}
	return 0;
}

int read_server_packet(int sock, server_packet *packet) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4) == -1) return -1;
	size = ntohl(size);

	uint8_t op;
	if (read_to_end(sock, (char *) &op, 1) == -1) return -1;
	switch (op) {
		case CONNECT:
			break;
		default:
			return -1;
	}
	
	return 0;
}

int write_string(int sock, const char *str) {
	int len = strlen(str);
	uint32_t size = htonl(len);
	// cout << "Writing " << len << " bytes as " << size << endl;
	if (write_all(sock, (char *) &size, 4) == -1) return -1;
	if (write_all(sock, str, len) == -1) return -1;
	return 0;
}

// Packeted string is uint32_t (4 bytes) + data
int string_size(const char *str) {
	return 4 + strlen(str);
}

uint32_t client_packet_size(const client_packet *packet) {
	switch (packet->op) {
		case CONNECT: {
			int login_size = string_size(packet->args.connect.login);
			int password_size = string_size(packet->args.connect.password);
			return 1 + login_size + password_size;
		}
		default:
			return (uint32_t) -1;
	}
}

int write_client_packet(int sock, const client_packet *packet) {
	uint32_t size = client_packet_size(packet);
	if (size == (uint32_t) -1) return -1;
	size = htonl(size);
	if (write_all(sock, (const char *) &size, 4) == -1) return -1;
	uint8_t op = packet->op;
	if (write_all(sock, (const char *) &op, 1) == -1) return -1;
	switch (op) {
		case CONNECT:
			if (write_string(sock, packet->args.connect.login) == -1) return -1;
			if (write_string(sock, packet->args.connect.password) == -1) return -1;
			break;
		default:
			return -1;
	}
	return 0;
}
