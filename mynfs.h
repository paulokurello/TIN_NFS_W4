struct mynfs_connection;

enum mynfs_errorcode {
    NONE = 0,
};

static int mynfs_error = NONE;

int mynfs_connect(mynfs_connection* conn, const char *host);
int mynfs_open(mynfs_connection* conn, const char *path, int oflag, int mode);

