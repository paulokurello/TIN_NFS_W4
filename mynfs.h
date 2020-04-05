struct mynfs_connection {
	int socket;
};

enum mynfs_errorcode {
    NONE = 0,
    UNKNOWN = 1,
    INVALID_ARGUMENTS = 2,
};

static int mynfs_error = NONE;

int mynfs_connect(mynfs_connection* conn, const char *host, const char *login, const char *password);
int mynfs_open(mynfs_connection* conn, const char *path, int oflag, int mode);
