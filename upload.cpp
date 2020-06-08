#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "mynfs.h"

#define try_op(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

using namespace std;

// Usage: upload host login pass local_from remote_to
int main(int argc, const char **argv) {
	if (argc < 6) return -1;
	mynfs_connection *conn;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "connect failed");
	int local = open(argv[4], O_RDONLY, 0);
	int remote;
	try_op((remote = mynfs_open(conn, argv[5], OF_WRONLY | OF_CREAT | OF_TRUNC, 0707)), "open failed");
	char buf[256] = {0};
	try_op(mynfs_lseek(conn, remote, 0, SEEK_SET), "lseek failed");
	int n;
	while((n = read(local, buf, sizeof(buf))) > 0) {
		try_op(mynfs_write(conn, remote, buf, n), "write failed");
	}
	close(local);
	try_op(mynfs_close(conn, remote), "close failed");
	try_op(mynfs_disconnect(conn), "disconnect failed");
	cout << "ok" << endl;
}