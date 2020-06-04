#include <iostream>
#include <cstring>
#include "mynfs.h"

using namespace std;

int main(int argc, char *argv[]) {
	mynfs_connection *conn;
	cout << "Connecting" << endl;
	if (mynfs_connect(&conn, "127.0.0.1", "admexter", "hunter51") == -1) {
		cout << "Got mynfs error on connection: " << mynfs_error << endl;
		return -1;
	}
	cout << "Opening" << endl;
	int fd;
	if ((fd = mynfs_open(conn, "test.txt", O_RDWR, 0)) == -1) {
		cout << "Got mynfs error on open: " << mynfs_error << endl;
		return -1;
	}
	cout << "Got fd: " << fd << endl;
	cout << "Reading" << endl;
	char buf[256] = {0};
	int size;
	if ((size = mynfs_read(conn, fd, buf, sizeof(buf))) == -1) {
		cout << "Got mynfs error on read: " << mynfs_error << endl;
		return -1;
	}
	cout << "Got " << size << " bytes: " << buf << endl;
	cout << "Seeking" << endl;
	if (mynfs_lseek(conn, fd, 0, SEEK_SET) == -1) {
		cout << "Got mynfs error on seek: " << mynfs_error << endl;
		return -1;
	}
	cout << "Writing" << endl;
	for (int i = 0; i < 4; i++) {
		buf[i] = "test"[i];
	}
	cout << "Sending: ";
	for (int i = 0; i < 4; i++) {
		cout << buf[i];
	}
	cout << endl;
	if (mynfs_write(conn, fd, buf, 4) == -1) {
		cout << "Got mynfs error on write: " << mynfs_error << endl;
		return -1;
	}
	cout << "Closing" << endl;
	if (mynfs_close(conn, fd) == -1) {
		cout << "Got mynfs error on close: " << mynfs_error << endl;
		return -1;
	}
	cout << "done" << endl;
	return 0;
}