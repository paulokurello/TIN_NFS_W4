#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <cstring>

#include "packet.cpp"

using namespace std;

void error(const char *msg, int code) {
	perror(msg);
	exit(code);
}

struct User {
	int id;
	string name;
	string password;
};

struct Users {
	vector<User> users;
	Users(const char *file_path) {
		ifstream file(file_path);
		while(!file.eof()) {
			User user;
			file >> user.id;
			file >> user.name;
			file >> user.password;
			this->users.push_back(user);
		}
	}
};

int check_client_pass(int sock, Users *users);
void handle_client(int sock);
void *spawn_client_thread(void *args);
void *spawn_fs_thread(void *args);

static bool running = true;
void sig_handler(int sig) {
	if (sig == SIGINT) {
		running = false;
		printf("Interrupt handled\n");
	}
	return;
}

void install_sighandler() {
	struct sigaction new_act;
	new_act.sa_handler = &sig_handler;
	new_act.sa_flags = 0;
	if (sigemptyset(&new_act.sa_mask) == -1)
		error("sigemptyset error", 1);
	if (sigaddset(&new_act.sa_mask, SIGINT) == -1)
		error("sigaddset error", 1);
	if (sigaction(SIGINT, &new_act, nullptr) == -1)
		error("Sigaction error", 1);
}

int init_sock() {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		error("Socket error", 1);
	}
	int enable = 1;
	if (setsockopt(sock, 0, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		error("Socket option error", 1);
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6996);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (sockaddr*) &addr, sizeof(addr))) {
		error("Bind error", 1);
	}

	if (listen(sock, 5) == -1) {
		error("Listen error", 1);
	}

	return sock;
}

int main(int argc, char* argv[]) {
	// Handle Ctrl-C to properly stop server
	install_sighandler();

	Users users("users");

	pthread_t fs;
	if (pthread_create(&fs, nullptr, spawn_fs_thread, nullptr) == -1)
		error("fs thread spawn", 1);

	int sock = init_sock();

	cout << "listening..." << endl;
	while (running) {

		sockaddr_in peer_addr;
		unsigned int peer_addr_len = sizeof(peer_addr);
		int peer_sock = accept(sock, (sockaddr*) &peer_addr, &peer_addr_len);
		if (peer_sock == -1) {
			cout << "Closing server..." << endl;
			close(sock);
			error("Accept error", 3);
		}
		if (peer_addr_len == sizeof(peer_addr))
			cout << "Connection from " << inet_ntoa(peer_addr.sin_addr)
			<< ":" << ntohs(peer_addr.sin_port) << endl;
		
		int user_id = check_client_pass(peer_sock, &users);;
		if (user_id == -1) {
			cout << "Error on checking password" << endl;
			close(peer_sock);
			continue;
		} else {
			cout << "password correct" << endl;
		}

		pthread_t thread;
		if (pthread_create(&thread, nullptr, spawn_client_thread, (void *) &peer_sock) == -1) {
			cout << "Error on handling client" << endl;
		}
	}
	cout << "Closing server..." << endl;
	close(sock);
	return 0;
}

void *spawn_fs_thread(void *args) {

	return nullptr;
}

void *spawn_client_thread(void *args) {
	handle_client(*(int *)args);
	return nullptr;
}

int check_client_pass(int sock, Users *users) {
	client_packet packet;
	if (read_client_packet(sock, &packet) == -1) {
		cout << "Read error" << endl;
		return -1;
	}
	if (packet.op != CONNECT) {
		cout << "Not a connection packet: " << packet.op << " 0x";
		cout << hex << packet.op << dec << endl;
		return -1;
	}

	server_packet s_packet;
	s_packet.res = (uint8_t) -1;
	int id = 0;
	for (User &user : users->users) {
		if (user.name == packet.args.connect.login) {
			if (user.password == packet.args.connect.password) {
				s_packet.res = 0;
				id = user.id;
				break;
			} else {
				cout << "Invalid password for " << packet.args.connect.login << endl;
				id = -1;
				break;
			}
		}
	}

	if (id == 0) {
		cout << "Unknown user: " << packet.args.connect.login << endl;
		id = -1;
	}

	if (write_server_packet(sock, &s_packet, CONNECT) == -1) {
		cout << "Write error" << endl;
		return -1;
	}

	return id;
}

void handle_client(int sock) {
	return;
}