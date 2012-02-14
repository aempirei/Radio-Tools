#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>

#define SZ 16

double l[SZ]={0.0};
double r[SZ]={0.0};

void shift(double *d, double v) {
	int i;
	for(i = 0; i < SZ - 1; i++) {
		d[i] = d[i + 1];
	}

	d[i] = v;
}

double mean(double *d) {
	double m = 0;
	int i;
	for(i = 0; i < SZ; i++)
		m += d[i];
	return m / SZ;
}

void print32(u_int32_t v) {
	ssize_t n = fwrite(&v, 1, sizeof(v), stdout);
	assert(n == sizeof(v));
}

void print16(u_int16_t v) {
	ssize_t n = fwrite(&v, 1, sizeof(v), stdout);
	assert(n = sizeof(v));
}

int main() {

	const int rate = 192000;
	const int length = 3840000;

	double dl;
	double dr;
	int n;
	short v;

	// write the wav header

	fputs("RIFF", stdout);      // magic
	print32(36 + length);       // chunksize
	fputs("WAVE", stdout);      // format

	fputs("fmt ", stdout);      // subchunk id
	print32(16);                // subchunk size
	print16(1);                 // audioformat (pcm)
	print16(1);                 // channels (mono)
	print32(rate);              // samplerate
	print32(rate * 1 * 2);      // byterate
	print16(1 * 2);             // blockalign
	print16(16);                // bitspersample

	fputs("data", stdout);      // subchunk id
	print32(length);            // subchunk size

	while(1) {
		n = read(0, &dl, sizeof(dl));
		if(n == 0) {
			break;
		} else if(n == -1) {
			perror("read()");
			exit(EXIT_FAILURE);
		}
		n = read(0, &dr, sizeof(dr));
		if(n == 0) {
			break;
		} else if(n == -1) {
			perror("read()");
			exit(EXIT_FAILURE);
		}
		v=(int)rint(dl*5.0*32767.0);
		n=fwrite(&v, sizeof(v), 1, stdout);
		if(n!=1) {
			perror("write()");
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
}
