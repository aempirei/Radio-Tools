#ifndef SHARED_H
#define SHARED_H
ssize_t write2(int fd, void *buf, size_t count);
int read2(int fd, void *data, size_t len);
void * loadfile(const char *fn, size_t *len);
int savefile(const char *fn, void *data, size_t len);
void fatal(const char *format, ...);
int bitcount(unsigned long v);
char * hzstring(double v, int res, char *buf, size_t buf_sz);
long long hztoll(const char *str);

typedef struct complex_sample {
	double r;
	double i;
} complex_sample_t;

#define MAX(a,b)	((a)>(b)?(a):(b))
#define MIN(a,b)	((a)<(b)?(a):(b))

#endif
