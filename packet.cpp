#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include "packet.h"

using namespace std;

#define try_op(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

//! Packet marshalling

int read_to_end(int sock, char *buf, int size) {
	int left = size;
	while (left > 0) {
		int n = read(sock, buf, left);
		if (n == 0) return -1;
		if (n < 0) {
			cout << "read_to_end error: " << strerror(errno) << endl;
			return -1;
		}
		left -= n;
		buf += n;
	}
	return 0;
}

int write_all(int sock, const char *buf, int size) {
	int left = size;
	while (left > 0) {
		int n = write(sock, buf, left);
		if (n == 0) return -1;
		if (n < 0) {
			cout << "write_all error: " << strerror(errno) << endl;
			return -1;
		} 
		left -= n;
		buf += n;
	}
	return 0;
}

int read_u32(int sock, uint32_t *value) {
	if (read_to_end(sock, (char *) value, 4) == -1) return -1;
	*value = ntohl(*value);
	return 0;
}

int read_string(int sock, char **str) {
	uint32_t size;
	if (read_u32(sock, &size) == -1) return -1;
	*str = (char *) malloc(size + 1);
	if (read_to_end(sock, *str, size) == -1) return -1;
	(*str)[size] = 0;
	return 0;
}

int read_stat(int sock, fd_stat *stat) {
	if (read_string(sock, &stat->name) == -1) return -1;
	if (read_u32(sock, &stat->mode) == -1) return -1;
	if (read_u32(sock, &stat->uid) == -1) return -1;
	if (read_u32(sock, &stat->size) == -1) return -1;
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
		case DISCONNECT: return 1;
		default: return (uint32_t) -1;
	}
}

int read_client_packet(int sock, client_packet *packet) {
	uint32_t size;
	if (read_u32(sock, &size) == -1) return -1;

	try_op(read_to_end(sock, (char *) &packet->op, 1), "Reading op error");
	switch (packet->op) {
		case CONNECT:
			try_op(read_string(sock, &packet->args.connect.login), "Reading login error");
			try_op(read_string(sock, &packet->args.connect.password), "Reading password error");
			break;
		case OPEN:
			try_op(read_string(sock, &packet->args.open.path), "Reading open path");
			try_op(read_u32(sock, &packet->args.open.oflag), "Reading open flag error");
			packet->args.open.oflag &= OF_LEGAL;
			try_op(read_u32(sock, &packet->args.open.mode), "Reading open mode error");
			break;
		case CLOSE:
			try_op(read_u32(sock, &packet->args.close.fd), "Reading close fd error");
			break;
		case READ:
			try_op(read_u32(sock, &packet->args.read.fd), "Reading read fd error");
			try_op(read_u32(sock, &packet->args.read.size), "Reading read size error");
			break;
		case WRITE:
			try_op(read_u32(sock, &packet->args.write.fd), "Reading write fd error");
			try_op(read_u32(sock, &packet->args.write.size), "Reading write size error");
			packet->args.write.data = malloc(packet->args.write.size);
			try_op(read_to_end(sock, (char *) packet->args.write.data, packet->args.write.size), "Reading write data error");
			break;
		case LSEEK:
			try_op(read_u32(sock, &packet->args.lseek.fd), "Reading lseek fd error");
			try_op(read_u32(sock, &packet->args.lseek.offset), "Reading lseek offset error");
			try_op(read_u32(sock, &packet->args.lseek.whence), "Reading lseek whence error");
			break;
		case UNLINK:
			try_op(read_string(sock, &packet->args.unlink.path), "Reading unlink path");
			break;
		case OPENDIR:
			try_op(read_string(sock, &packet->args.opendir.path), "Reading opendir path");
			break;
		case READDIR:
			try_op(read_u32(sock, &packet->args.readdir.dir_fd), "Reading readdir dir_fd error");
			break;
		case CLOSEDIR:
			try_op(read_u32(sock, &packet->args.closedir.dir_fd), "Reading closedir dir_fd error");
			break;
		case FSTAT:
			try_op(read_u32(sock, &packet->args.fstat.fd), "Reading fstat fd error");
			break;
		case STAT:
			try_op(read_string(sock, &packet->args.stat.path), "Reading stat path");
			break;
		case DISCONNECT: break;
		default:
			cout << "[read_client_packet] Unknown op: " << (int) packet->op << endl;
			return -1;
	}
	if (client_packet_size(packet) != size) {
		cout << "Expected size " << client_packet_size(packet) << endl;
		return -1;
	}
	return 0;
}

uint32_t server_packet_size(const server_packet *packet, int op) {
	switch (op) {
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
		case FSTAT: return 1 + sizeof(fd_stat);
		case STAT: return 1 + sizeof(fd_stat);
		case DISCONNECT: return 1;
		default: return (uint32_t) -1;
	}
	return (uint32_t) -1;
}

int read_server_packet(int sock, server_packet *packet, int op) {
	uint32_t size;
	try_op(read_u32(sock, &size), "Reading size error");

	uint8_t res;
	try_op(read_to_end(sock, (char *) &res, 1), "Reading res error");
	switch (op) {
		case CONNECT: break;
		case OPEN:
			try_op(read_u32(sock, &packet->ret.open.fd), "Reading open fd error");
			break;
		case CLOSE: break;
		case READ:
			try_op(read_u32(sock, &packet->ret.read.size), "Reading read size error");
			packet->ret.read.data = malloc(packet->ret.read.size);
			if (packet->ret.read.data == nullptr) return -1;
			try_op(read_to_end(sock, (char *) packet->ret.read.data, packet->ret.read.size), "Reading read data error");
			break;
		case WRITE: break;
		case LSEEK:
			try_op(read_u32(sock, &packet->ret.lseek.offset), "Reading lseek offset error");
			break;
		case UNLINK: break;
		case OPENDIR:
			try_op(read_u32(sock, &packet->ret.opendir.dir_fd), "Reading opendir dir_fd error");
			break;
		case READDIR:
			try_op(read_string(sock, &packet->ret.readdir.name), "Reading readdir name error");
			break;
		case CLOSEDIR: break;
		case FSTAT:
			try_op(read_stat(sock, &packet->ret.fstat.stat), "Reading fstat stat error");
			break;
		case STAT:
			try_op(read_stat(sock, &packet->ret.stat.stat), "Reading stat stat error");
			break;
		case DISCONNECT: break;
		default:
			cout << "[read_server_packet] Unknown op: " << (int) op << endl;
			return -1;
	}
	if (server_packet_size(packet, op) != size) return -1;
	
	return 0;
}

int write_u32(int sock, uint32_t value) {
	value = htonl(value);
	if (write_all(sock, (const char *) &value, sizeof(value)) == -1) return -1;
	return 0;
}

int write_string(int sock, const char *str) {
	uint32_t len = strlen(str);
	// cout << "Writing " << len << " bytes as " << size << endl;
	if (write_u32(sock, len) == -1) return -1;
	if (write_all(sock, str, len) == -1) return -1;
	return 0;
}

int write_stat(int sock, fd_stat stat) {
	if (write_string(sock, stat.name) == -1) return -1;
	if (write_u32(sock, stat.mode) == -1) return -1;
	if (write_u32(sock, stat.uid) == -1) return -1;
	if (write_u32(sock, stat.size) == -1) return -1;
	return 0;
}

int write_client_packet(int sock, const client_packet *packet) {
	uint32_t size = client_packet_size(packet);
	if (size == (uint32_t) -1) return -1;
	if (write_u32(sock, size) == -1) return -1;
	uint8_t op = packet->op;
	if (write_all(sock, (const char *) &op, 1) == -1) return -1;
	switch (op) {
		case CONNECT:
			try_op(write_string(sock, packet->args.connect.login), "Writing connect login error");
			try_op(write_string(sock, packet->args.connect.password), "Writing connect password error");
			break;
		case OPEN:
			try_op(write_string(sock, packet->args.open.path), "Writing open path error");
			try_op(write_u32(sock, packet->args.open.oflag & OF_LEGAL), "Writing open oflag error");
			try_op(write_u32(sock, packet->args.open.mode), "Writing open mode error");
			break;
		case CLOSE:
			try_op(write_u32(sock, packet->args.close.fd), "Writing close fd error");
			break;
		case READ:
			try_op(write_u32(sock, packet->args.read.fd), "Writing read fd error");
			try_op(write_u32(sock, packet->args.read.size), "Writing read size error");
			break;
		case WRITE:
			try_op(write_u32(sock, packet->args.write.fd), "Writing write fd error");
			try_op(write_u32(sock, packet->args.write.size), "Writing write size error");
			try_op(write_all(sock, (const char *) packet->args.write.data, packet->args.write.size), "Writing write data error");
			break;
		case LSEEK:
			try_op(write_u32(sock, packet->args.lseek.fd), "Writing lseek fd error");
			try_op(write_u32(sock, packet->args.lseek.offset), "Writing lseek offset error");
			try_op(write_u32(sock, packet->args.lseek.whence), "Writing lseek whence error");
			break;
		case UNLINK:
			try_op(write_string(sock, packet->args.unlink.path), "Writing unlink path error");
			break;
		case OPENDIR:
			try_op(write_string(sock, packet->args.opendir.path), "Writing opendir path error");
			break;
		case READDIR:
			try_op(write_u32(sock, packet->args.readdir.dir_fd), "Writing readdir dir_fd error");
			break;
		case CLOSEDIR:
			try_op(write_u32(sock, packet->args.closedir.dir_fd), "Writing closedir dir_fd error");
			break;
		case FSTAT:
			try_op(write_u32(sock, packet->args.fstat.fd), "Writing fstat fd error");
			break;
		case STAT:
			try_op(write_string(sock, packet->args.stat.path), "Writing stat path error");
			break;
		case DISCONNECT: break;
		default:
			return -1;
	}
	return 0;
}

// In server packet, op is "implicit", i.e. is based on previous packet
// E.g. if server receives packet with READ in op, it will send back read packet with data
int write_server_packet(int sock, const server_packet *packet, int op) {
	uint32_t size = server_packet_size(packet, op);
	if (size == (uint32_t) -1) return -1;
	if (write_u32(sock, size) == -1) return -1;
	uint8_t res = packet->res;
	if (write_all(sock, (const char *) &res, 1) == -1) return -1;
	switch (op) {
		case CONNECT: break;
		case OPEN: 
			try_op(write_u32(sock, packet->ret.open.fd), "Writing open fd error");
			break;
		case CLOSE: break;
		case READ: 
			try_op(write_u32(sock, packet->ret.read.size), "Writing read size error");
			try_op(write_all(sock, (const char *) packet->ret.read.data, packet->ret.read.size), "Writing read data error");
			break;
		case WRITE: break;
		case LSEEK: 
			try_op(write_u32(sock, packet->ret.lseek.offset), "Writing lseek offset error");
			break;
		case UNLINK: break;
		case OPENDIR: 
			try_op(write_u32(sock, packet->ret.opendir.dir_fd), "Writing opendir dir_fd error");
			break;
		case READDIR: 
			try_op(write_string(sock, packet->ret.readdir.name), "Writing readdir name error");
			break;
		case CLOSEDIR: break;
		case FSTAT: 
			try_op(write_stat(sock, packet->ret.fstat.stat), "Writing fstat stat error");
			break;
		case STAT: 
			try_op(write_stat(sock, packet->ret.stat.stat), "Writing stat stat error");
			break;
		case DISCONNECT: break;
		default:
			return -1;
	}
	return 0;
}