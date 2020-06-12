#ifndef MYNFS_H
#define MYNFS_H

#include "packet.h"

struct mynfs_connection;

// int mynfs_error = NONE;

const int OK = 0;
const int ERROR = -1;

int mynfs_connect(mynfs_connection** conn,
	const char *host, const char *login, const char *password);
int mynfs_disconnect(mynfs_connection *conn);

int mynfs_open(mynfs_connection* conn,
	const char *path, int oflag, int mode);
int mynfs_close(mynfs_connection* conn, int fd);
int mynfs_read(mynfs_connection* conn, int fd, char *buf, int size);
int mynfs_write(mynfs_connection* conn, int fd, const char *buf, int size);
int mynfs_lseek(mynfs_connection* conn, int fd, int offset, int whence);

int mynfs_unlink(mynfs_connection* conn, const char *path);
int mynfs_opendir(mynfs_connection* conn, const char *path);
const char *mynfs_readdir(mynfs_connection* conn, int dir_fd);
int mynfs_closedir(mynfs_connection* conn, int dir_fd);
int mynfs_fstat(mynfs_connection* conn, int fd, fd_stat *stat);
int mynfs_stat(mynfs_connection* conn, const char *path, fd_stat *stat);

#endif
