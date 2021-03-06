#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>
#include <unordered_map>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <cstring>
#include <fcntl.h>
#include <dirent.h>

#include "packet.cpp"
#include "queues.cpp"

using namespace std;

const char *FILE_SHARE = "NFS/";
const char *FILE_DB = "files.txt";
const char *USERS_DB = "users.txt";

void error(const char *msg, int code) {
	perror(msg);
	exit(code);
}

char *copy_str(const char *str) {
	if (str == nullptr) return nullptr;
	unsigned size = strlen(str);
	char *ret = (char *) malloc(size + 1);
	for (unsigned i = 0; i < size; i++)
		ret[i] = str[i];
	ret[size] = 0;
	return ret;
}

struct User {
	string name;
	string password;
};

struct Users {
	unordered_map<int, User> users;
	Users(const char *file_path) {
		ifstream file{file_path};
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
void handle_client(int sock, int uid);
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

	Users users(USERS_DB);

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
			cout << "wrong password" << endl;
			close(peer_sock);
			continue;
		} else {
			cout << "password correct" << endl;
		}

		pthread_t thread;
		int args[2] = {peer_sock, user_id};
		if (pthread_create(&thread, nullptr, spawn_client_thread, (void *) args) == -1) {
			cout << "Error on handling client" << endl;
		}
	}
	cout << "Closing server..." << endl;
	close(sock);
	return 0;
}

void *spawn_fs_thread(void *args) {
	struct FileDb {
		// name => (uid, perms)
		map<string, pair<int, int>> files;

		FileDb(const char *file_path) {
			ifstream file{file_path};
			while(!file.eof()) {
				string name;
				file >> name;
				if (name.empty()) continue;
				int uid;
				file >> uid;
				int perms;
				file >> oct >> perms >> dec;
				this->files.insert({name, {uid, perms}});
			}
		}

		void save(const char *file_path) {
			ofstream file{file_path};
			for (auto [name, second] : files) {
				auto [uid, perms] = second;
				if (name == string("tree")) continue;
				file << name << " " << uid << " 0" << oct << perms << dec << endl;
			}
		}

		void gen_tree(string *tree) {
			ostringstream s;
			s << "        ";
			for (auto file : files) {
				auto name = file.first;
				if (name == string("tree")) continue;
				s << name << endl;
			}
			*tree = s.str();
		}

		bool has_perms(string path, int c_uid, int c_perms) {
			// TODO: Check perms better
			for (auto [name, second] : files) {
				auto [uid, perms] = second;
				if (path == name && (c_uid == uid || c_uid == 0)) return true;
			}
			return false;
		}

		optional<fd_stat> get_stat(string path) {
			fd_stat fd_stat;
			auto find = files.find(path);
			if (find == files.end()) return nullopt;
			auto [name, second] = *find;
			auto [uid, perms] = second;
			fd_stat.name = copy_str(name.c_str());
			fd_stat.uid = uid;
			fd_stat.mode = perms;
			if ((perms & IS_DIR) != 0) fd_stat.size = 0;
			else {
				string file_path = FILE_SHARE;
				file_path += path;
				struct stat file_stat;
				if (stat(file_path.c_str(), &file_stat) != 0) return nullopt;
				fd_stat.size = file_stat.st_size;
			}
			return fd_stat;
		}
	};

	struct OpenFile {
		string path;
		int local_fd;
		int uid;
	};

	struct OpenDir {
		DIR *local_dir_fd;
		int uid;
	};

	FileDb file_db(FILE_DB);
	// remote_fd => (local_fd, uid)
	unordered_map<int, OpenFile> open_files{};
	unsigned int counter = 2;
	unordered_map<int, OpenDir> open_dirs{};
	unsigned int dir_counter = 1;

	string tree = "";
	file_db.gen_tree(&tree);

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
		Response response;
		response.thread_id = request.thread_id;
		response.packet.res = 1; // Default to error
		switch(request.packet.op) {
			case OPEN: {
				cout << "Trying to open: " << request.packet.args.open.path << endl;
				int local_fd;
				if (request.packet.args.open.path == string("tree")) {
					local_fd = 0;
				} else {
					if (!file_db.has_perms(
						request.packet.args.open.path,
						request.uid,
						request.packet.args.open.mode)
						&& !(request.packet.args.open.oflag & O_CREAT))
					{
						break;
					}
					string path = FILE_SHARE;
					path += request.packet.args.open.path;
					local_fd = open(
						path.c_str(),
						request.packet.args.open.oflag,
						request.packet.args.open.mode
					);
					if (local_fd < 0) break;
				}
				int fd = counter;
				if (counter == 0) counter = 2;
				else counter += 1;

				file_db.files.insert({request.packet.args.open.path, {request.uid, request.packet.args.open.mode}});
				open_files.insert({fd, {request.packet.args.open.path, local_fd, request.uid}});
				response.packet.res = 0;
				response.packet.ret.open.fd = fd;
				file_db.save(FILE_DB);
				file_db.gen_tree(&tree);
				break;
			}
			case CLOSE: {
				auto file = open_files.find(request.packet.args.close.fd);
				if (file != open_files.end()) {
					if (file->second.uid == request.uid) {
						close(file->second.local_fd);
						open_files.erase(file);
						response.packet.res = 0;
					}
				}
				break;
			}
			case READ: {
				auto file = open_files.find(request.packet.args.read.fd);
				if (file != open_files.end()) {
					if (file->second.uid == request.uid) {
						void *buf = malloc(request.packet.args.read.size);
						int n;
						if (file->second.path == string("tree")) {
							string_view view = tree.substr(file->second.local_fd);
							n = min(request.packet.args.read.size, (unsigned) view.size());
							char *c_buf = (char *) buf;
							for (int i = 0; i < n; i++) {
								c_buf[i] = view[i];
							}
							file->second.local_fd += n;
						} else {
							n = read(file->second.local_fd, buf, request.packet.args.read.size);
						}
						if (n >= 0) {
							response.packet.res = 0;
							response.packet.ret.read.size = n;
							response.packet.ret.read.data = buf;
						}
					}
				}
				break;
			}
			case WRITE: {
				auto file = open_files.find(request.packet.args.write.fd);
				if (file != open_files.end()) {
					if (file->second.uid == request.uid) {
						int n = write(
							file->second.local_fd,
							(const char*) request.packet.args.write.data,
							request.packet.args.write.size
						);
						if (n >= 0) {
							response.packet.res = 0;
							response.packet.ret.write.size = n;
						} 
					}
				}
				break;
			}
			case LSEEK: {
				auto file = open_files.find(request.packet.args.lseek.fd);
				if (file != open_files.end()) {
					if (file->second.uid == request.uid) {
						int n = lseek(
							file->second.local_fd,
							request.packet.args.lseek.offset,
							request.packet.args.lseek.whence
						);
						if (n >= 0) {
							response.packet.res = 0;
							response.packet.ret.lseek.offset = n;
						}
					}
				}
				break;
			}
			case OPENDIR: {
				if (!file_db.has_perms(
					request.packet.args.open.path,
					request.uid,
					request.packet.args.open.mode))
				{
					response.packet.res = 1;
					break;
				}
				string path = FILE_SHARE;
				path += request.packet.args.opendir.path;
				DIR *local_dir_fd = opendir(path.c_str());
				if (local_dir_fd == nullptr) break;
				int dir_fd = dir_counter;
				dir_counter += 1;
				if (dir_counter == 0) dir_counter = 1;
				open_dirs.insert({dir_fd, {local_dir_fd, request.uid}});
				response.packet.res = 0;
				response.packet.ret.opendir.dir_fd = dir_fd;
				break;
			}
			case READDIR: {
				cout << "Trying readdir" << endl;
				auto dir = open_dirs.find(request.packet.args.readdir.dir_fd);
				if (dir != open_dirs.end()) {
					if (dir->second.uid == request.uid) {
						cout << "Trying readdir" << endl;
						auto entry = readdir(dir->second.local_dir_fd);
						// Readdir error
						if (entry == nullptr && errno != 0) {
							cout << "Shiz whack, yo!" << endl;
							break;
						}
						char *name = nullptr;
						if (entry) {
							cout << "Entry: " << entry->d_name << endl;
							name = copy_str(entry->d_name);
						}

						response.packet.res = 0;
						response.packet.ret.readdir.name = name;
					}
				}
				break;
			}
			case CLOSEDIR: {
				auto dir = open_dirs.find(request.packet.args.closedir.dir_fd);
				if (dir != open_dirs.end()) {
					if (dir->second.uid == request.uid) {
						closedir(dir->second.local_dir_fd);
						open_dirs.erase(dir);
						response.packet.res = 0;
					}
				}
				break;
			}
			case UNLINK: {
				if (file_db.has_perms(request.packet.args.unlink.path, request.uid, 202)) {
					file_db.files.erase(request.packet.args.unlink.path);
					string name = FILE_SHARE;
					name += request.packet.args.unlink.path;
					remove(name.c_str());
					file_db.save(FILE_DB);
					file_db.gen_tree(&tree);
				}
				break;
			}
			case FSTAT: {
				response.packet.ret.fstat.stat.name = NULL;
				auto file = open_files.find(request.packet.args.fstat.fd);
				if (file != open_files.end()) {
					if (file->second.uid == request.uid) {
						auto stat = file_db.get_stat(file->second.path);
						if (stat) {
							response.packet.res = 0;
							response.packet.ret.fstat.stat = *stat;
						}
					}
				}
				break;
			}
			case STAT: {
				response.packet.ret.stat.stat.name = NULL;
				if (file_db.has_perms(request.packet.args.stat.path, request.uid, 404)) {
					auto stat = file_db.get_stat(request.packet.args.stat.path);
					if (stat) {
						response.packet.res = 0;
						response.packet.ret.fstat.stat = *stat;
					}
				}
				break;
			}
			default: continue;
		}
		response_queue.push(response);
	}
	return nullptr;
}

void *spawn_client_thread(void *vargs) {
	int *args = (int *) vargs;
	handle_client(args[0], args[1]);
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
		s_packet.res = 0;
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

void handle_client(int sock, int uid) {
	pthread_t thread_id = pthread_self();
	bool running = true;
	while (running) {
		client_packet packet;
		server_packet s_packet;
		cout << "Receiving... ";
		if(read_client_packet(sock, &packet) == -1) {
			cout << "[handle_client] Read error" << endl;
			return;
		}
		cout << "got " << op_name(packet.op) << endl;
		switch(packet.op) {
			case OPEN:
			case CLOSE:
			case READ:
			case WRITE:
			case LSEEK:
			case UNLINK:
			case OPENDIR:
			case READDIR:
			case CLOSEDIR:
			case FSTAT:
			case STAT:
				request_queue.push({thread_id, uid, packet});
				Response response;
				while(true) {
					auto pop = response_queue.remove([thread_id](Response req){
						return req.thread_id == thread_id;
					});
					if (pop) {
						response = *pop;
						break;
					}
					pthread_yield();
				}
				s_packet = response.packet;
				break;
			case DISCONNECT:
				s_packet.res = 0;
				running = false;
				break;
			default:
				cout << "[handle_client] Invalid op" << (int) packet.op << endl;
				return;
		}
		cout << "Sending " << op_name(packet.op) << "... " << endl;
		if (write_server_packet(sock, &s_packet, packet.op) == -1) {
			cout << "[handle_client] Write error" << endl;
			return;
		}
	}
	cout << "Client disconnect" << endl;
	return;
}
