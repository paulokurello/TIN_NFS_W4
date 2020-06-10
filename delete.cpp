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

// Usage: delete host login pass name
int main(int argc, const char *argv[]) {
	if (argc < 5) return -1;
	mynfs_connection *conn;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "connect failed");
	try_op(mynfs_unlink(conn, argv[4]), "unlink failed");
	try_op(mynfs_disconnect(conn), "disconnect failed");
	cout << "ok" << endl;
}