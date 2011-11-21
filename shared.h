ssize_t write2(int fd, void *buf, size_t count);
int read2(int fd, void *data, size_t len);
void * loadfile(const char *fn, size_t *len);
int savefile(const char *fn, void *data, size_t len);
