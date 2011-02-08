# $OpenBSD: Makefile,v 1.23 2010/10/19 08:13:59 espie Exp $

COMMENT=		small tiling window manager

SHARED_LIBS=		twmhack 0.0

DISTNAME=		torba-TORBAVERSION
EXTRACT_SUFX=		.tgz
CATEGORIES=		x11

HOMEPAGE=		http://www.torba.org/
MAINTAINER=		Marco Peereboom <marco@peereboom.us>

# BSD
PERMIT_PACKAGE_CDROM=   Yes
PERMIT_PACKAGE_FTP=     Yes
PERMIT_DISTFILES_CDROM= Yes
PERMIT_DISTFILES_FTP=   Yes

MASTER_SITES=	http://opensource.conformal.com/snapshots/torba/

NO_REGRESS=     Yes
WANTLIB=        c X11 Xrandr util

RUN_DEPENDS=	:dmenu->=3.9:x11/dmenu
		
MAKE_FLAGS=	BINDIR="${TRUEPREFIX}/bin" \
		MANDIR="${TRUEPREFIX}/man/cat" \
		SHLIB_MAJOR=${LIBtwmhack_VERSION:C/\..*//} \
		SHLIB_MINOR=${LIBtwmhack_VERSION:C/.*\.//}
USE_GROFF =	Yes

pre-configure:
	@${SUBST_CMD} ${WRKSRC}/torba.conf ${WRKSRC}/torba.c

post-install:
	${INSTALL_DATA_DIR} ${PREFIX}/share/examples/torba
	${INSTALL_SCRIPT} ${WRKSRC}/baraction.sh ${PREFIX}/share/examples/torba
	${INSTALL_SCRIPT} ${WRKSRC}/initscreen.sh ${PREFIX}/share/examples/torba
	${INSTALL_SCRIPT} ${WRKSRC}/screenshot.sh ${PREFIX}/share/examples/torba
	${INSTALL_DATA} ${WRKSRC}/torba.conf ${PREFIX}/share/examples/torba

.for i in es it pt ru
	${INSTALL_DATA_DIR} ${PREFIX}/man/$i/cat1
	mv ${PREFIX}/man/cat1/torba_$i.0 ${PREFIX}/man/$i/cat1/torba.0
.endfor

.include <bsd.port.mk>
