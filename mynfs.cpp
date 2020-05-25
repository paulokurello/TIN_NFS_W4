#include <iostream>
#include <sys/socket.h>

#include "mynfs.h"
#include "packet.cpp"

using namespace std;

struct mynfs_connection {
    int socket;
};

const int OK = 0;
const int ERROR = -1;

int mynfs_connect(mynfs_connection** connection, const char *host, const char *login, const char *password) {
	*connection = (mynfs_connection *) malloc(sizeof(mynfs_connection));
	mynfs_connection* conn = *connection;
	mynfs_error = 3;
	cout << "Connecting to " << host << endl;

	conn->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (conn->socket == -1) {
		perror("Socket error");
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6996);
	if (inet_aton(host, &addr.sin_addr) == 0) {
		mynfs_error = INVALID_ARGUMENTS;
		return ERROR;
	}

	if (connect(conn->socket, (sockaddr*) &addr, sizeof(addr))) {
		perror("Connect error");
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	client_packet packet;
	packet.op = CONNECT;
	packet.args.connect.login = (char *) login;
	packet.args.connect.password = (char *) password;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet) == -1) {
		cout << "Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "Response error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	cout << "ok" << endl;

	return OK;
}

int mynfs_open(mynfs_connection* conn, const char *path, int oflag, int mode) {
    cout << "Opening file " << path << endl;
    mynfs_error = 1;
    return ERROR;
}

