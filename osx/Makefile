# $torba: Makefile,v 1.5 2010/09/16 02:00:51 marco Exp $ 

CFLAGS+= -O2 -Wall -ggdb -D_GNU_SOURCE -D__OSX__ -I.
CFLAGS+= -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+=  -L/usr/X11/lib/ -lX11 -lXrandr

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

CC= gcc

LVERS= $(shell . ../lib/shlib_version; echo $$major.$$minor)

all: torba libswmhack.so.$(LVERS)

torba.c:
	ln -sf ../torba.c

swm_hack.c:
	ln -sf ../lib/swm_hack.c

torba: torba.o osx.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

%.so: %.c
	$(CC) $(CFLAGS) -c -fpic -DPIC $+ -o $@

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) -shared -fpic -o libswmhack.so.$(LVERS) $(LDADD) swm_hack.so
# replace above line with this for OSX 10.5
#	$(CC) -shared -bundle -fpic -o libswmhack.so.$(LVERS) $(LDADD) swm_hack.so

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 torba $(DESTDIR)$(BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(DESTDIR)$(LIBDIR)
	install -m 644 ../torba.1 $(DESTDIR)$(MANDIR)/man1/torba.1
	install -m 644 ../torba_es.1 $(DESTDIR)$(MANDIR)/man1/torba_es.1
	install -m 644 ../torba_it.1 $(DESTDIR)$(MANDIR)/man1/torba_it.1
	install -m 644 ../torba_pt.1 $(DESTDIR)$(MANDIR)/man1/torba_pt.1
	install -m 644 ../torba_ru.1 $(DESTDIR)$(MANDIR)/man1/torba_ru.1

clean:
	rm -f torba *.o *.so libswmhack.so.* torba.c swm_hack.c

.PHONY: all install clean
