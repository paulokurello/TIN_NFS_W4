#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#include "mynfs.h"

#define try_op(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

using namespace std;

// Usage: download host login pass remote_from local_to
int main(int argc, const char **argv) {
	if (argc < 6) return -1;
	mynfs_connection *conn;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "connect failed");
	int remote;
	try_op((remote = mynfs_open(conn, argv[4], OF_RDONLY, 0)), "open failed");
	int local = open(argv[5], O_WRONLY | O_CREAT | O_TRUNC, 0777);
	try_op(local, strerror(errno));
	char buf[256] = {0};
	int n;
	while((n = mynfs_read(conn, remote, buf, sizeof(buf))) > 0) {
		write(local, buf, n);
	}
	try_op(n, "read failed");
	close(local);
	try_op(mynfs_close(conn, remote), "close failed");
	try_op(mynfs_disconnect(conn), "disconnect failed");
	cout << "ok" << endl;
}