CC = gcc
CCFLAGS = -Wall -I. -lm -lfftw3 -g -ggdb
CFLAGS = -Wall -I.  -g -ggdb
TARGETS = bandpass_filter differentiate integrate abs decode scale power

.PHONY: all clean wipe

all: $(TARGETS) $(MODULES) $(SCRIPTS)

integrate: integrate.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

differentiate: differentiate.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

bandpass_filter: bandpass_filter.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

abs: abs.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

decode: decode.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

scale: scale.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

power: power.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

clean:
	rm -f *.o *~

wipe: clean
	rm -f $(TARGETS)
