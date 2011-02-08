#!/bin/sh
# $torba: initscreen.sh,v 1.2 2009/09/13 22:28:53 marco Exp $
#
# Example xrandr multiscreen init

xrandr --output LVDS --auto
xrandr --output VGA --auto --right-of LVDS
