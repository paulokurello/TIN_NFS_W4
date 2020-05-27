#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

#define try(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

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
    uint32_t mode;
    uint32_t uid;
    uint32_t size;
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
			uint32_t oflag;
			uint32_t mode;
		} open;
		struct {
			uint32_t fd;
		} close;
		struct {
			uint32_t fd;
			uint32_t size;
		} read;
		struct {
			uint32_t fd;
			uint32_t size;
			void *data;
		} write;
		struct {
			uint32_t fd;
			uint32_t offset;
			uint32_t whence;
		} lseek;
		struct {
			char *path;
		} unlink;
		struct {
			char *path;
		} opendir;
		struct {
			uint32_t dir_fd;
		} readdir;
		struct {
			uint32_t dir_fd;
		} closedir;
		struct {
			uint32_t fd;
		} fstat;
		struct {
			char *path;
		} stat;
	} args;
};

struct server_packet {
	int res;
	union {
		struct {} connect;
		struct {
			uint32_t fd;
		} open;
		struct {} close;
		struct {
			uint32_t size;
			void *data;
		} read;
		struct {} write;
		struct {
			uint32_t offset;
		} lseek;
		struct {} unlink;
		struct {
			uint32_t dir_fd;
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
	} ret;
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

int read_u32(int sock, uint32_t *value) {
	if (read_to_end(sock, (char *) value, 4) == -1) return -1;
	*value = ntohl(*value);
	return 0;
}
int read_stat(int sock, packet_stat *stat) {
	if (read_string(sock, &stat->name) == -1) return -1;
	if (read_u32(sock, &stat->mode) == -1) return -1;
	if (read_u32(sock, &stat->uid) == -1) return -1;
	if (read_u32(sock, &stat->size) == -1) return -1;
	return 0;
}

int read_client_packet(int sock, client_packet *packet) {
	uint32_t size;
	if (read_u32(sock, &size) == -1) return -1;

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

	try(read_to_end(sock, (char *) &packet->op, 1), "Reading op error");
	switch (packet->op) {
		case CONNECT:
			try(read_string(sock, &packet->args.connect.login), "Reading login error");
			try(read_string(sock, &packet->args.connect.password), "Reading password error");
			break;
		case OPEN:
			try(read_string(sock, &packet->args.open.path), "Reading open path");
			try(read_u32(sock, &packet->args.open.oflag), "Reading open flag error");
			try(read_u32(sock, &packet->args.open.mode), "Reading open mode error");
			break;
		case CLOSE:
			try(read_u32(sock, &packet->args.close.fd), "Reading close fd error");
			break;
		case READ:
			try(read_u32(sock, &packet->args.read.fd), "Reading read fd error");
			try(read_u32(sock, &packet->args.read.size), "Reading read size error");
			break;
		case WRITE:
			try(read_u32(sock, &packet->args.write.fd), "Reading write fd error");
			try(read_u32(sock, &packet->args.write.size), "Reading write size error");
			packet->args.write.data = malloc(packet->args.write.size);
			try(read_to_end(sock, (char *) &packet->args.write.data, size), "Reading write data error");
			break;
		case LSEEK:
			try(read_u32(sock, &packet->args.lseek.fd), "Reading lseek fd error");
			try(read_u32(sock, &packet->args.lseek.offset), "Reading lseek offset error");
			try(read_u32(sock, &packet->args.lseek.whence), "Reading lseek whence error");
			break;
		case UNLINK:
			try(read_string(sock, &packet->args.unlink.path), "Reading unlink path");
			break;
		case OPENDIR:
			try(read_string(sock, &packet->args.opendir.path), "Reading opendir path");
			break;
		case READDIR:
			try(read_u32(sock, &packet->args.readdir.dir_fd), "Reading readdir dir_fd error");
			break;
		case CLOSEDIR:
			try(read_u32(sock, &packet->args.closedir.dir_fd), "Reading closedir dir_fd error");
			break;
		case FSTAT:
			try(read_u32(sock, &packet->args.fstat.fd), "Reading fstat fd error");
			break;
		case STAT:
			try(read_string(sock, &packet->args.stat.path), "Reading stat path");
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
		case CONNECT: break;
		case OPEN:
			try(read_u32(sock, &packet->ret.open.fd), "Reading open fd error");
			break;
		case CLOSE: break;
		case READ:
			try(read_u32(sock, &packet->ret.read.size), "Reading read size error");
			packet->ret.read.data = malloc(packet->ret.read.size);
			try(read_to_end(sock, (char *) &packet->ret.read.data, size), "Reading read data error");
			break;
		case WRITE: break;
		case LSEEK:
			try(read_u32(sock, &packet->ret.lseek.offset), "Reading lseek offset error");
			break;
		case UNLINK: break;
		case OPENDIR:
			try(read_u32(sock, &packet->ret.opendir.dir_fd), "Reading opendir dir_fd error");
			break;
		case READDIR:
			try(read_string(sock, &packet->ret.readdir.name), "Reading readdir name error");
			break;
		case CLOSEDIR: break;
		case FSTAT:
			try(read_stat(sock, &packet->ret.fstat.stat), "Reading fstat stat error");
			break;
		case STAT:
			try(read_stat(sock, &packet->ret.stat.stat), "Reading stat stat error");
			break;
		default: return -1;
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

int write_u32(int sock, uint32_t value) {
	if (write_all(sock, (const char *) &value, sizeof(value)) == -1) return -1;
	return 0;
}

int write_stat(int sock, packet_stat stat) {
	if (write_string(sock, stat.name) == -1) return -1;
	if (write_u32(sock, stat.mode) == -1) return -1;
	if (write_u32(sock, stat.uid) == -1) return -1;
	if (write_u32(sock, stat.size) == -1) return -1;
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
		case OPEN: return 1 + string_size(packet->args.open.path) + 4 + 4;
		case CLOSE: return 1 + 4;
		case READ: return 1 + 4 + 4;
		case WRITE: return 1 + 4 + 4 + packet->args.write.size;
		case LSEEK: return 1 + 4 + 4 + 4;
		case UNLINK: return 1 + string_size(packet->args.unlink.path);
		case OPENDIR: return 1 + string_size(packet->args.opendir.path);
		case READDIR: return 1 + 4;
		case CLOSEDIR: return 1 + 4;
		case FSTAT: return 1 + 4;
		case STAT: return 1 + string_size(packet->args.stat.path);
		default: return (uint32_t) -1;
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
			try(write_string(sock, packet->args.connect.login), "Writing connect login error");
			try(write_string(sock, packet->args.connect.password), "Writing connect password error");
			break;
		case OPEN:
			try(write_string(sock, packet->args.open.path), "Writing open path error");
			try(write_u32(sock, packet->args.open.oflag), "Writing open oflag error");
			try(write_u32(sock, packet->args.open.mode), "Writing open mode error");
			break;
		case CLOSE:
			try(write_u32(sock, packet->args.close.fd), "Writing close fd error");
			break;
		case READ:
			try(write_u32(sock, packet->args.read.fd), "Writing read fd error");
			try(write_u32(sock, packet->args.read.size), "Writing read size error");
			break;
		case WRITE:
			try(write_u32(sock, packet->args.write.fd), "Writing write fd error");
			try(write_u32(sock, packet->args.write.size), "Writing write size error");
			try(write_all(sock, (const char *) packet->args.write.data, packet->args.write.size), "Writing write data error");
			break;
		case LSEEK:
			try(write_u32(sock, packet->args.lseek.fd), "Writing lseek fd error");
			try(write_u32(sock, packet->args.lseek.offset), "Writing lseek offset error");
			try(write_u32(sock, packet->args.lseek.whence), "Writing lseek whence error");
			break;
		case UNLINK:
			try(write_string(sock, packet->args.unlink.path), "Writing unlink path error");
			break;
		case OPENDIR:
			try(write_string(sock, packet->args.opendir.path), "Writing opendir path error");
			break;
		case READDIR:
			try(write_u32(sock, packet->args.readdir.dir_fd), "Writing readdir dir_fd error");
			break;
		case CLOSEDIR:
			try(write_u32(sock, packet->args.closedir.dir_fd), "Writing closedir dir_fd error");
			break;
		case FSTAT:
			try(write_u32(sock, packet->args.fstat.fd), "Writing fstat fd error");
			break;
		case STAT:
			try(write_string(sock, packet->args.stat.path), "Writing stat path error");
			break;
		default:
			return -1;
	}
	return 0;
}

uint32_t server_packet_size(const server_packet *packet) {
	switch (packet->res) {
		case CONNECT: return 1;
		case OPEN: return 1 + 4;
		case CLOSE: return 1;
		case READ: return 1 + 4 + packet->ret.read.size;
		case WRITE: return 1;
		case LSEEK: return 1 + 4;
		case UNLINK: return 1;
		case OPENDIR: return 1 + 4;
		case READDIR: return 1 + string_size(packet->ret.readdir.name);
		case CLOSEDIR: return 1;
		case FSTAT: return 1 + sizeof(packet_stat);
		case STAT: return 1 + sizeof(packet_stat);
		default: return (uint32_t) -1;
	}
	return (uint32_t) -1;
}

int write_server_packet(int sock, const server_packet *packet) {
	uint32_t size = server_packet_size(packet);
	if (size == (uint32_t) -1) return -1;
	size = htonl(size);
	if (write_all(sock, (const char *) &size, 4) == -1) return -1;
	uint8_t res = packet->res;
	if (write_all(sock, (const char *) &res, 1) == -1) return -1;
	switch (res) {
		case CONNECT: break;
		case OPEN: 
			try(write_u32(sock, packet->ret.open.fd), "Writing open fd error");
			break;
		case CLOSE: break;
		case READ: 
			try(write_u32(sock, packet->ret.read.size), "Writing read size error");
			try(write_all(sock, (const char *) packet->ret.read.data, packet->ret.read.size), "Writing read data error");
			break;
		case WRITE: break;
		case LSEEK: 
			try(write_u32(sock, packet->ret.lseek.offset), "Writing lseek offset error");
			break;
		case UNLINK: break;
		case OPENDIR: 
			try(write_u32(sock, packet->ret.opendir.dir_fd), "Writing opendir dir_fd error");
			break;
		case READDIR: 
			try(write_string(sock, packet->ret.readdir.name), "Writing readdir name error");
			break;
		case CLOSEDIR: break;
		case FSTAT: 
			try(write_stat(sock, packet->ret.fstat.stat), "Writing fstat stat error");
			break;
		case STAT: 
			try(write_stat(sock, packet->ret.stat.stat), "Writing stat stat error");
			break;
		default:
			return -1;
	}
	return 0;
}