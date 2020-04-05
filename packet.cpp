#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

typedef enum {
	CONNECT = 1,
	OPEN = 2,
} op_code;

struct client_packet {
	op_code op;
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
	} args;
};

struct server_packet {
	int res;
	union ret {
		struct {} connect;
		struct {
			int fd;
		} open;
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

int read_string(int sock, char *str) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4) == -1) return -1;
	size = ntohs(size);
	cout << "Reading " << size << " bytes" << endl;
	str = (char *) malloc(size + 1);
	if (read_to_end(sock, str, size) == -1) return -1;
	str[size] = 0;
	return 0;
}

int read_client_packet(int sock, client_packet *packet) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4) == -1) return -1;
	size = ntohs(size);

	cout << size << endl;
	char *buf = (char *) malloc(size);
	read_to_end(sock, buf, size);
	for (int i = 0; i < size; i++) {
		cout << "0x" << hex << (int) buf[i] << dec << endl;
	}
	free(buf);
	// uint8_t op;
	// if (read_to_end(sock, (char *) &op, 1) == -1) return -1;
	// switch (op) {
	// 	case CONNECT:
	// 		if (read_string(sock, packet->args.connect.login) == -1) return -1;
	// 		if (read_string(sock, packet->args.connect.password) == -1) return -1;
	// 		break;
	// 	default:
	// 		return -1;
	// }
	return 0;
}

int read_server_packet(int sock, server_packet *packet) {
	uint32_t size;
	if (read_to_end(sock, (char *) &size, 4) == -1) return -1;
	size = ntohs(size);

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
	uint32_t size = htons(len);
	cout << "Writing " << len << " bytes as " << size << endl;
	if (write_all(sock, (char *) &size, 4) == -1) return -1;
	if (write_all(sock, str, len) == -1) return -1;
	return 0;
}

// Packeted string is uint32_t (4 bytes) + data
int string_size(const char *str) {
	return 4 + strlen(str);
}

int client_packet_size(const client_packet *packet) {
	switch (packet->op) {
		case CONNECT: {
			int login_size = string_size(packet->args.connect.login);
			int password_size = string_size(packet->args.connect.password);
			return 1 + login_size + password_size;
		}
		default:
			return -1;
	}
}

int write_client_packet(int sock, const client_packet *packet) {
	uint32_t size = client_packet_size(packet);
	if (size == -1) return -1;
	size = htons(size);
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
