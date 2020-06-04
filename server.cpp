#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <cstring>

#include "packet.cpp"
#include "queues.cpp"

using namespace std;

void error(const char *msg, int code) {
	perror(msg);
	exit(code);
}

struct User {
	string name;
	string password;
};

struct Users {
	unordered_map<int, User> users;
	Users(const char *file_path) {
		ifstream file(file_path);
		while(!file.eof()) {
			int id;
			User user;
			file >> id;
			file >> user.name;
			file >> user.password;
			this->users.insert({id, user});
		}
	}

	optional<int> has(string login, string password) {
		for (auto &[id, user] : this->users) {
			if (user.name == login && user.password == password) {
				return id;
			}
		}
		return nullopt;
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

static MutexQueue<Request> request_queue;
static MutexQueue<Response> response_queue;

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
	struct FileDb {
		unordered_map<string, pair<int, int>> files;
		FileDb(const char *file_path) {
			ifstream file(file_path);
			while(!file.eof()) {
				string name;
				file >> name;
				int uid;
				file >> uid;
				int perms;
				file >> perms;
				this->files.insert({name, {uid, perms}});
			}
		}
	};

	struct OpenFile {
		int local_fd;
		int uid;
	};

	FileDb file_db("files.txt");
	unordered_map<int, OpenFile> open_files();

	while(true) {
		Request request;
		while(true) {
			auto pop = request_queue.pop();
			if (pop) {
				request = *pop;
				break;
			}
			pthread_yield();
		}
		switch(request.packet.op) {
			case OPEN:

				break;
			default: continue;
		}
	}
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
	if (auto oid = users->has(packet.args.connect.login, packet.args.connect.password)) {
		id = *oid;
	}

	if (id == 0) {
		cout << "Invalid login info for: " << packet.args.connect.login << endl;
		id = -1;
	}

	if (write_server_packet(sock, &s_packet, CONNECT) == -1) {
		cout << "Write error" << endl;
		return -1;
	}

	return id;
}

void handle_client(int sock) {
	pthread_t thread_id = pthread_self();
	while (true) {
		client_packet packet;
		server_packet s_packet;
		cout << "Receiving... ";
		if(read_client_packet(sock, &packet) == -1) {
			cout << "[handle_client] Read error" << endl;
			return;
		}
		cout << "ok" << endl;
		switch(packet.op) {
			case OPEN:
			case CLOSE:
			case READ:
			case WRITE:
			case LSEEK:
				request_queue.push({thread_id, packet});
				Response response;
				while(true) {
					auto pop = response_queue.remove([thread_id](Response req){ return req.thread_id == thread_id; });
					if (pop) {
						response = *pop;
						break;
					}
					pthread_yield();
				}
				break;
			case UNLINK: break;
			case OPENDIR: break;
			case READDIR: break;
			case CLOSEDIR: break;
			case FSTAT: break;
			case STAT: break;
			case KEEPALIVE:
				s_packet.res = 0;
				break;
			default:
				cout << "[handle_client] Invalid op" << (int) packet.op << endl;
				return;
		}
		cout << "Sending " << (int) packet.op << "... ";
		if (write_server_packet(sock, &s_packet, packet.op) == -1) {
			cout << "[handle_client] Write error" << endl;
			return;
		}
		cout << "ok" << endl;
	}
	return;
}