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

// Usage: create host login pass (-d) name
int main(int argc, const char *argv[]) {
	if (argc < 5) return -1;
	bool mkdir = (strcmp(argv[4], "-d") == 0);
	if (mkdir && argc < 6) return -1;

	mynfs_connection *conn;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "connect failed");
	int fd;
	int flags = O_CREAT | (mkdir ? O_DIRECTORY : 0);
	try_op((fd = mynfs_open(conn, argv[mkdir ? 5 : 4], flags, 0)), "open failed");
	try_op(mynfs_close(conn, fd), "close failed");
	try_op(mynfs_disconnect(conn), "disconnect failed");
	cout << "ok" << endl;
}