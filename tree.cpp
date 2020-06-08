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
	try_op((fd = mynfs_opendir(conn, path.c_str())), "");
	fd_stat stat;
	try_op(mynfs_fstat(conn, fd, &stat), "");
	spaces(level * 2);
	cout << stat.name << endl;
	const char *entry_path;
	while((entry_path = mynfs_readdir(conn, fd)) != NULL) {
		try_op(mynfs_stat(conn, entry_path, &stat), "");
		if ((stat.mode & IS_DIR) != 0) {
			try_op(stat_dir(conn, entry_path, level + 1), "");
		} else {
			spaces((level + 1) * 2);
			cout << stat.name << " (size " << stat.size << ")" << endl;
		}
	}
	try_op(mynfs_closedir(conn, fd), "");
	return 0;
}

// Usage: tree host login pass
int main(int argc, const char **argv) {
	if (argc < 4) return -1;
	mynfs_connection *conn;
	try_op(mynfs_connect(&conn, argv[1], argv[2], argv[3]), "");
	stat_dir(conn, "/", 0);
	try_op(mynfs_disconnect(conn), "");
	cout << "ok" << endl;
}