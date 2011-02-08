# $torba: Makefile,v 1.16 2010/09/16 02:00:51 marco Exp $
.include <bsd.xconf.mk>

PREFIX?=/usr/local

BINDIR=${PREFIX}/bin
SUBDIR= lib

PROG=torba
MAN=torba.1 torba_es.1 torba_it.1 torba_pt.1 torba_ru.1

CFLAGS+=-std=c89 -Wall -Wno-uninitialized -ggdb3
# Uncomment define below to disallow user settable clock format string
#CFLAGS+=-DSWM_DENY_CLOCK_FORMAT
CPPFLAGS+= -I${X11BASE}/include
LDADD+=-lutil -L${X11BASE}/lib -lX11 -lXrandr

MANDIR= ${PREFIX}/man/cat

torba_ru.cat1: torba_ru.1
	 nroff -mandoc ${.CURDIR}/torba_ru.1 > ${.TARGET}

obj: _xenocara_obj

.include <bsd.prog.mk>
.include <bsd.xorg.mk>
