struct mynfs_connection;

struct stat {
    char *name;
    int mode;
    int uid;
    int size;
};

enum mynfs_errorcode {
    NONE = 0,
    UNKNOWN = 1,
    INVALID_ARGUMENTS = 2,
};

enum oflags {
	O_RDONLY,
	O_WRONLY,
	O_RDWR,
	O_APPEND,
	O_CREAT,
	O_EXCL,
	O_TRUNC
};

int mynfs_error = NONE;

int mynfs_connect(mynfs_connection** conn,
	const char *host, const char *login, const char *password);
int mynfs_open(mynfs_connection* conn,
	const char *path, int oflag, int mode);
int mynfs_read(mynfs_connection* conn, int fd, void *buf, int size);
int mynfs_write(mynfs_connection* conn, int fd, const void *buf, int size);
int mynfs_lseek(mynfs_connection* conn, int fd, int offset, int whence);
int mynfs_close(mynfs_connection* conn, int fd);

int mynfs_unlink(mynfs_connection* conn, char *path);
int mynfs_opendir(mynfs_connection* conn, char *path);
char *mynfs_readdir(mynfs_connection* conn, int dirfd);
int mynfs_closedir(mynfs_connection* conn, int dirfd);
int mynfs_fstat(mynfs_connection* conn, int fd, stat *stat);
int mynfs_stat(mynfs_connection* conn, char *path, stat *stat);
