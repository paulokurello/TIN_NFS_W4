#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <climits>

#include "mynfs.h"

#define try_op(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

int write_all(int sock, const char *buf, int size) {
	int left = size;
	while (left > 0) {
		int n = write(sock, buf, left);
		if (n == 0) return -1;
		if (n < 0) {
			std::cout << "write_all error: " << strerror(errno) << std::endl;
			return -1;
		}
		left -= n;
		buf += n;
	}
	return 0;
}

using namespace std;

// Usage: download host login pass remote_from local_to (bytes_start bytes_amount)
int main(int argc, const char **argv) {
	if (argc < 6) return -1;
	unsigned long start = 0;
	unsigned long amount = INT_MAX;
	if (argc >= 7) {
		start = atol(argv[6]);
		amount = atol(argv[7]);
		cout << "Seeking to " << start << " and reading " << amount << " bytes" << endl;
	}
	mynfs_connection *conn;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "connect failed");
	int remote;
	try_op((remote = mynfs_open(conn, argv[4], OF_RDONLY, 0)), "open failed");
	try_op(mynfs_lseek(conn, remote, start, SEEK_SET), "lseek failed");
	int local = open(argv[5], O_WRONLY | O_CREAT, 0777);
	try_op(local, strerror(errno));
	try_op(lseek(local, start, SEEK_SET), "lseek failed");
	char buf[256] = {0};
	int n;
	while(amount > 0 && (n = mynfs_read(conn, remote, buf, min(sizeof(buf), amount))) > 0) {
		write_all(local, buf, n);
		amount -= n;
	}
	try_op(n, "read failed");
	close(local);
	try_op(mynfs_close(conn, remote), "close failed");
	try_op(mynfs_disconnect(conn), "disconnect failed");
	cout << "ok" << endl;
}