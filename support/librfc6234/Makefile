all:	librfc6234.a

librfc6234.a:	hkdf.o hmac.o sha1.o sha224-256.o sha384-512.o usha.o
	$(AR) r librfc6234.a hkdf.o hmac.o sha1.o sha224-256.o sha384-512.o usha.o

hkdf.o:	hkdf.c sha.h
	$(CC) $(CFLAGS) -c -o hkdf.o hkdf.c

hmac.o:	hmac.c sha.h
	$(CC) $(CFLAGS) -c -o hmac.o hmac.c

sha1.o:	sha1.c sha.h sha-private.h
	$(CC) $(CFLAGS) -c -o sha1.o sha1.c

sha224-256.o:	sha224-256.c sha.h sha-private.h
	$(CC) $(CFLAGS) -c -o sha224-256.o sha224-256.c

sha384-512.o:	sha384-512.c sha.h sha-private.h
	$(CC) $(CFLAGS) -c -o sha384-512.o sha384-512.c

usha.o:	usha.c sha.h
	$(CC) $(CFLAGS) -c -o usha.o usha.c

clean:
	rm -f librfc6234.a hkdf.o hmac.o sha1.o sha224-256.o sha384-512.o usha.o
