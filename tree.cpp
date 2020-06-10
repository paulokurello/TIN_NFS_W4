#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#include "mynfs.h"

#define try_op(op, msg) {\
	if ((op) == -1) {\
		cout << (msg) << endl;\
		return -1;\
	}\
}

using namespace std;

void spaces(int level) { for (;level > 0; level--) cout << " "; }

int stat_dir(mynfs_connection *conn, string path, int level) {
	int fd;
	// try_op((fd = mynfs_open(conn, path.c_str(), O_DIRECTORY | O_RDONLY, 0)), "open failed");
	fd_stat stat;
	try_op(mynfs_stat(conn, path.c_str(), &stat), "stat failed");
	spaces(level * 2);
	cout << stat.name << endl;
	// try_op(mynfs_close(conn, fd), "fstat failed");
	try_op((fd = mynfs_opendir(conn, path.c_str())), "opendir failed");
	const char *entry_path;
	while((entry_path = mynfs_readdir(conn, fd)) != NULL) {
		string new_path = path;
		new_path += "/";
		new_path += entry_path;
		try_op(mynfs_stat(conn, new_path.c_str(), &stat), "stat failed");
		if ((stat.mode & IS_DIR) != 0) {
			try_op(stat_dir(conn, new_path, level + 1), "stat_dir failed");
		} else {
			spaces((level + 1) * 2);
			cout << stat.name << " (size " << stat.size << ")" << endl;
		}
	}
	try_op(mynfs_closedir(conn, fd), "closedir failed");
	return 0;
}

// Usage: tree host login pass
int main(int argc, const char **argv) {
	if (argc < 4) return -1;
	mynfs_connection *conn;
	cout << "Connecting..." << endl;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "connect failed");
	try_op(stat_dir(conn, "/", 0), "stat_dir failed");
	try_op(mynfs_disconnect(conn), "disconnect failed");
	cout << "ok" << endl;
}