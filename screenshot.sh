#!/bin/sh
#
# $scrotwm: screenshot.sh,v 1.3 2009/09/13 22:28:53 marco Exp $

screenshot() {
	case $1 in
	full)
		scrot -m
		;;
	window)
		sleep 1
		scrot -s
		;;
	*)
		;;
	esac;
}

screenshot $1
