CC=gcc
CFLAGS=-O3 -I../testvectors -Wall -Wextra -std=c89 -pedantic -Wno-long-long
BLAKEBINS=blake2s blake2b blake2sp blake2bp blake2xs blake2xb

all:		$(BLAKEBINS) check

blake2s:	blake2s.c
		$(CC) blake2s.c -o $@ $(CFLAGS) -DBLAKE2S_SELFTEST

blake2b:	blake2b.c
		$(CC) blake2b.c -o $@ $(CFLAGS) -DBLAKE2B_SELFTEST

blake2sp:	blake2sp.c blake2s.c
		$(CC) blake2sp.c blake2s.c -o $@ $(CFLAGS) -DBLAKE2SP_SELFTEST

blake2bp:	blake2bp.c blake2b.c
		$(CC) blake2bp.c blake2b.c -o $@ $(CFLAGS) -DBLAKE2BP_SELFTEST

blake2xs:	blake2xs.c blake2s.c
		$(CC) blake2xs.c blake2s.c -o $@ $(CFLAGS) -DBLAKE2XS_SELFTEST

blake2xb:	blake2xb.c blake2b.c
		$(CC) blake2xb.c blake2b.c -o $@ $(CFLAGS) -DBLAKE2XB_SELFTEST

check:          blake2s blake2b blake2sp blake2bp blake2xs blake2xb
	        ./blake2s
	        ./blake2b
	        ./blake2sp
	        ./blake2bp
	        ./blake2xs
	        ./blake2xb

kat:
		$(CC) $(CFLAGS) -o genkat-c genkat-c.c blake2b.c blake2s.c blake2sp.c blake2bp.c blake2xs.c blake2xb.c
		$(CC) $(CFLAGS) -g -o genkat-json genkat-json.c blake2b.c blake2s.c blake2sp.c blake2bp.c blake2xs.c blake2xb.c
		./genkat-c > blake2-kat.h
		./genkat-json > blake2-kat.json

clean:
		rm -rf *.o genkat-c genkat-json blake2-kat.h blake2-kat.json $(BLAKEBINS)
