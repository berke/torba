#	$torba: Makefile,v 1.3 2009/10/02 19:23:48 marco Exp $

.include <bsd.xconf.mk>

PREFIX?=/usr/local

LIB=    twmhack
NOMAN=	yes
SRCS=	twm_hack.c
LIBDIR= ${X11BASE}/lib

DEBUGLIBS= no
NOPROFILE= yes

CFLAGS+=-Wall -Wno-uninitialized -ggdb3
CFLAGS+= -I${X11BASE}/include

install:
	${INSTALL} ${INSTALL_COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
		${PREFIX}/lib/

.include <bsd.lib.mk>
.include <bsd.xorg.mk>
