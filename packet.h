#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

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
	KEEPALIVE = 13,
};

enum errorcode {
    NONE = 0,
    UNKNOWN = 1,
    INVALID_ARGUMENTS = 2,
};

struct fd_stat {
    char *name;
    uint32_t mode;
    uint32_t uid;
    uint32_t size;
};

struct client_packet {
	uint8_t op;
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
		struct {} keepalive;
	} args;
};

struct server_packet {
	uint8_t res;
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
			fd_stat stat;
		} fstat;
		struct {
			fd_stat stat;
		} stat;
		struct {} keepalive;
	} ret;
};

#endif
