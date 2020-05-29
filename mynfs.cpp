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
		perror("[mynfs_connection] Socket error");
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
		perror("[mynfs_connection] Connect error");
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	client_packet packet;
	packet.op = CONNECT;
	packet.args.connect.login = (char *) login;
	packet.args.connect.password = (char *) password;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_connection] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, CONNECT) == -1) {
		cout << "[mynfs_connection] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_connection] Response error: " << recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_open(mynfs_connection* conn, const char *path, int oflag, int mode) {
    cout << "Opening file " << path << endl;
    
	client_packet packet;
	packet.op = OPEN;
	packet.args.open.path = (char *) path;
	packet.args.open.oflag = oflag;
	packet.args.open.mode = mode;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_open] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, OPEN) == -1) {
		cout << "[mynfs_open] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_open] Response error: " << recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return recv_packet.ret.open.fd;
}

int mynfs_close(mynfs_connection* conn, int fd) {
	return ERROR;
}

int mynfs_read(mynfs_connection* conn, int fd, void *buf, int size) {
	return ERROR;
}

int mynfs_write(mynfs_connection* conn, int fd, const void *buf, int size) {
	return ERROR;
}
