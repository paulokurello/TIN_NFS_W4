#include "mynfs.h"
#include <iostream>
#include <sys/socket.h>

using namespace std;

struct mynfs_connection {
    int socket;
};

const int OK = 0;
const int ERROR = -1;

int mynfs_connect(mynfs_connection* conn, const char *host) {
	cout << "Connecting to " << host << endl;
	return OK;
}

int mynfs_open(mynfs_connection* conn, const char *path, int oflag, int mode) {
    cout << "Opening file " << path << endl;
    mynfs_error = 1;
    return ERROR;
}

