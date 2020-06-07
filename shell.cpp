#include <iostream>
#include <string>

#include "mynfs.h"

using namespace std;

#define try_op(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

bool command(string_view input, const char *name) {
	return input.find(name) == 0;
}

string_view arg(string_view input, int nth) {
	int start = 0;
	for (; nth > 0; nth--) {
		start = input.find(' ', start);
		start = input.find_first_not_of(' ', start);
	}
	if (start == (int) string::npos) return {};
	int end = input.find(' ', start);
	return input.substr(start, end == (int) string::npos ? input.size() : end);
}

struct Args {
	string host, login, password;
	Args(int argc, const char **argv) {
		if (argc == 4) {
			host = argv[1];
			login = argv[2];
			password = argv[3];
		} else if (argc == 1) {
			cout << "Host: "; cin >> host;
			cout << "Login: "; cin >> login;
			cout << "Password: "; cin >> password;
		} else {
			cout << "Usage " << argv[0] << " ([host] [login] [password])" << endl;
			exit(1);
		}
	}
};

int main(int argc, const char **argv) {
	mynfs_connection *conn;
	Args args{argc, argv};
	try_op(mynfs_connect(&conn, args.host.c_str(), args.login.c_str(), args.password.c_str()), "Couldn't connect");
	cout << "Use \"help\" to list available command" << endl;
	while (true) {
		cout << "> ";
		string input;
		cin >> input;
		if (command(input, "exit")) {
			break;
		} else if(command(input, "help")) {
			cout << "Available command: exit, help, ls, cat" << endl;
		} else if(command(input, "ls")) {

		} else if(command(input, "cat")) {
			string path = string(arg(input, 1));
			int fd;
			try_op((fd = mynfs_open(conn, path.c_str(), OF_RDONLY, 0)), "Open fail");
			char buf[256] = {0};
			int n = 1;
			do {
				try_op((n = mynfs_read(conn, fd, buf, sizeof(buf))), "Read fail");
				for (int i = 0; i < n; i++) {
					cout << buf[i];
				}
			} while(n > 0);
			cout << endl;
			try_op(mynfs_close(conn, fd), "Close fail");
		} else {
			cout << "Unknown command" << endl;
		}
	}
	try_op(mynfs_disconnect(conn), "Couldn't disconnect properly");

	cout << "Goodbye" << endl;
	return 0;
}