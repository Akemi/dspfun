CC ?= cc
CFLAGS := -std=c11 -Ofast $(CFLAGS)
LIBS := -I../include $(shell pkg-config --cflags --libs MagickWand) -lm

all: applybasis genbasis draw

applybasis: applybasis.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

genbasis: genbasis.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

draw: draw.c
	$(CC) $(CFLAGS) $(LIBS) $(shell pkg-config --libs fftw3f) -o $@ $<

clean:
	rm -f draw genbasis applybasis

.PHONY: all clean
