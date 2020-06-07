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
	cout << "Res: " << (int) recv_packet.res << endl;
	if (recv_packet.res != 0) {
		cout << "[mynfs_connection] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_disconnect(mynfs_connection* conn) {
	client_packet packet;
	packet.op = DISCONNECT;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_open] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, DISCONNECT) == -1) {
		cout << "[mynfs_open] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_open] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return recv_packet.ret.open.fd;
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
		cout << "[mynfs_open] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return recv_packet.ret.open.fd;
}

int mynfs_close(mynfs_connection* conn, int fd) {
	client_packet packet;
	packet.op = CLOSE;
	packet.args.close.fd = fd;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_close] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, CLOSE) == -1) {
		cout << "[mynfs_close] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_close] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_read(mynfs_connection* conn, int fd, char *buf, int size) {
    
	client_packet packet;
	packet.op = READ;
	packet.args.read.fd = fd;
	packet.args.read.size = size;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_read] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, READ) == -1) {
		cout << "[mynfs_read] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_read] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	for (unsigned int i = 0; i < recv_packet.ret.read.size; i++) {
		buf[i] = ((char *) recv_packet.ret.read.data)[i];
	}
	// memcpy(buf, &recv_packet.ret.read.data, recv_packet.ret.read.size);

	return recv_packet.ret.read.size;
}

int mynfs_write(mynfs_connection* conn, int fd, char *data, int size) {
    
	client_packet packet;
	packet.op = WRITE;
	packet.args.write.fd = fd;
	packet.args.write.size = size;
	packet.args.write.data = (char *) data;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_write] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, WRITE) == -1) {
		cout << "[mynfs_write] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_write] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_lseek(mynfs_connection* conn, int fd, int offset, int whence) {
    
	client_packet packet;
	packet.op = LSEEK;
	packet.args.lseek.fd = fd;
	packet.args.lseek.offset = offset;
	packet.args.lseek.whence = whence;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_lseek] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, LSEEK) == -1) {
		cout << "[mynfs_lseek] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_lseek] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_unlink(mynfs_connection* conn, const char *path) {
    
	client_packet packet;
	packet.op = UNLINK;
	packet.args.unlink.path = (char *) path;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_unlink] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, UNLINK) == -1) {
		cout << "[mynfs_unlink] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_unlink] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_opendir(mynfs_connection* conn, const char *path) {
	cout << "Opening directory " << path << endl;
    
	client_packet packet;
	packet.op = OPENDIR;
	packet.args.opendir.path = (char *) path;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_opendir] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, OPENDIR) == -1) {
		cout << "[mynfs_opendir] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_opendir] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return recv_packet.ret.opendir.dir_fd;
}

const char *mynfs_readdir(mynfs_connection* conn, int fd) {
    
	client_packet packet;
	packet.op = READDIR;
	packet.args.readdir.dir_fd = fd;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_readdir] Write error" << endl;
		mynfs_error = UNKNOWN;
		return nullptr;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, READDIR) == -1) {
		cout << "[mynfs_readdir] Read error" << endl;
		mynfs_error = UNKNOWN;
		return nullptr;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_readdir] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return nullptr;
	}

	return recv_packet.ret.readdir.name;
}

int mynfs_closedir(mynfs_connection* conn, int fd) {
    
	client_packet packet;
	packet.op = CLOSEDIR;
	packet.args.closedir.dir_fd = fd;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_closedir] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, CLOSEDIR) == -1) {
		cout << "[mynfs_closedir] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_closedir] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_fstat(mynfs_connection* conn, int fd) {
    
	client_packet packet;
	packet.op = FSTAT;
	packet.args.fstat.fd = fd;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_fstat] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, FSTAT) == -1) {
		cout << "[mynfs_fstat] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_fstat] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}

int mynfs_stat(mynfs_connection* conn, const char *path) {
    
	client_packet packet;
	packet.op = STAT;
	packet.args.stat.path = (char *) path;
	if (write_client_packet(conn->socket, &packet) == -1) {
		cout << "[mynfs_stat] Write error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	
	server_packet recv_packet;
	if (read_server_packet(conn->socket, &recv_packet, STAT) == -1) {
		cout << "[mynfs_stat] Read error" << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}
	if (recv_packet.res != 0) {
		cout << "[mynfs_stat] Response error: " << (int) recv_packet.res << endl;
		mynfs_error = UNKNOWN;
		return ERROR;
	}

	return OK;
}