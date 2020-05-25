#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstring>

#include "packet.cpp"

using namespace std;

void error(const char *msg, int code) {
	perror(msg);
	exit(code);
}

int handle_client(int sock);

static bool running = true;
void sig_handler(int sig) {
	if (sig == SIGINT)
		running = false;
}

int main(int argc, char* argv[]) {
	// Handle Ctrl-C to properly stop server
	struct sigaction new_act;
	new_act.sa_handler = &sig_handler;
	new_act.sa_flags = 0;
	if (sigemptyset(&new_act.sa_mask) == -1)
		error("sigemptyset error", 1);
	if (sigaddset(&new_act.sa_mask, SIGINT) == -1)
		error("sigaddset error", 1);
	if (sigaction(SIGINT, &new_act, NULL) == -1)
		error("Sigaction error", 1);

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

	while (running) {
		cout << "listening..." << endl;

		sockaddr_in peer_addr;
		unsigned int peer_addr_len = sizeof(peer_addr);
		int peer_sock = accept(sock, (sockaddr*) &peer_addr, &peer_addr_len);
		if (peer_sock == -1) {
			error("Accept error", 3);
		}
		if (peer_addr_len == sizeof(peer_addr))
			cout << "Connection from " << inet_ntoa(peer_addr.sin_addr)
			<< ":" << ntohs(peer_addr.sin_port) << endl;
		
		if (handle_client(peer_sock) == -1) {
			cout << "Error on handling client" << endl;
		} else {
			cout << "ok" << endl;
		}

		close(peer_sock);
	}
	cout << "Closing server..." << endl;
	close(sock);
	return 0;
}

int handle_client(int sock) {
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
	cout << "Login: " << packet.args.connect.login;
	cout << " Password: " << packet.args.connect.password << endl;
	return 0;
}