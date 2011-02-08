#!/bin/sh
#
# $torba: screenshot.sh,v 1.3 2009/09/13 22:28:53 marco Exp $

screenshot() {
	case $1 in
	full)
		torb -m
		;;
	window)
		sleep 1
		torb -s
		;;
	*)
		;;
	esac;
}

screenshot $1
