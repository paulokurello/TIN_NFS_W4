#include <iostream>
#include <cstring>
#include "mynfs.h"

using namespace std;

int main(int argc, char *argv[]) {
	mynfs_connection *conn;
	if (mynfs_connect(&conn, "127.0.0.1", "adam", "hunter51") == -1) {
		cout << "Got mynfs error on connection: " << mynfs_error << endl;
		return -1;
	}
	cout << "ok" << endl;
	int fd;
	if ((fd = mynfs_open(conn, "/test.txt", 0, 0)) == -1) {
		cout << "Got mynfs error on open: " << mynfs_error << endl;
		return -1;
	}
	char buf[256] = {0};
	int size;
	if ((size = mynfs_read(conn, fd, buf, sizeof(buf))) == -1) {
		cout << "Got mynfs error on read: " << mynfs_error << endl;
		return -1;
	}
	cout << "Got " << size << " bytes: " << buf << endl;
	strcpy(buf, "test");
	if (mynfs_write(conn, fd, buf, 4) == -1) {
		cout << "Got mynfs error on write: " << mynfs_error << endl;
		return -1;
	}

	if (mynfs_close(conn, fd) == -1) {
		cout << "Got mynfs error on close: " << mynfs_error << endl;
		return -1;
	}
	return 0;
}