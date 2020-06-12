#include <iostream>
#include "mynfs.h"

using namespace std;

int main() {
	mynfs_connection *conn1;
	if(mynfs_connect(&conn1, "127.0.0.1", "a", "b") == ERROR)
		cout << "Test failed sucessfully" << endl;
	mynfs_connection *conn2;
	if(mynfs_connect(&conn2, "127.0.0.1", "admexter", "b") == ERROR)
		cout << "Test failed sucessfully" << endl;
	
	mynfs_connection *conn3;
	if(mynfs_connect(&conn3, "127.0.0.1", "monalisa", "davinci") == ERROR) {
		cout << "Test failed unsucessfully" << endl;
		return -1;
	}
	mynfs_connection *conn;
	if(mynfs_connect(&conn, "127.0.0.1", "admexter", "hunter51") == ERROR) {
		cout << "Test failed unsucessfully" << endl;
		return -1;
	}
	// char buf[4];
	// if(mynfs_read(conn, 1, buf, 4) == ERROR)
	// 	cout << "Test failed sucessfully" << endl;
	if(mynfs_open(conn, "../files.txt", O_RDWR, 0) == ERROR)
		cout << "Test failed sucessfully" << endl;
	int fd;
	if((fd = mynfs_open(conn, "test.txt", O_RDONLY, 0)) == ERROR) {
		cout << "Test failed unsucessfully" << endl;
		return -1;
	}
	if(mynfs_write(conn, fd, "nono", 4) == ERROR)
		cout << "Test failed sucessfully" << endl;
	if(mynfs_disconnect(conn) == ERROR) {
		cout << "Test failed unsucessfully" << endl;
		return -1;
	}
	if(mynfs_disconnect(conn3) == ERROR) {
		cout << "Test failed unsucessfully" << endl;
		return -1;
	}
	cout << "ok" << endl;
	return 0;
}