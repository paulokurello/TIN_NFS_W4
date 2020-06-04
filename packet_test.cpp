#include <fcntl.h>
#include "packet.h"
#include "packet.cpp"

int main() {
	int fd = open("test.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd == -1) {
		perror("error");
		return -1;
	}
	cout << "Got fd" << endl;
	server_packet s_packet;
	
	s_packet.res = 0;
	s_packet.ret.read.data = (void *) "test";
	s_packet.ret.read.size = 4;
	write_server_packet(fd, &s_packet, READ);
	cout << "Written" << endl;
	
	lseek(fd, 0, SEEK_SET);
	s_packet = {};
	if (read_server_packet(fd, &s_packet, READ) == -1) {
		cout << "r_error";
		return -1;
	}

	cout << "Got: ";
	char *data = (char *) &s_packet.ret.read.data;
	for (unsigned int i = 0; i < s_packet.ret.read.size; i++)
		cout << data[i];
	cout << endl;

	return 0;
}