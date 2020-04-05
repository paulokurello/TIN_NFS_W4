#include <iostream>
#include "mynfs.h"

using namespace std;

int main(int argc, char *argv[]) {
	mynfs_connection conn;
	if (mynfs_connect(&conn, "127.0.0.1", "adam", "hunter51") == -1) return -1;
	cout << "ok" << endl;
	return 0;
}