#!/bin/bash
#
# $Id$

# 64-bit wrapper for win32-setup.sh.

export DOWNLOAD_TAG="2009-03-06"
export PLATFORM="win64"

WIN32_SETUP=`echo $0 | sed -e s/win64/win32/`

exec $WIN32_SETUP $@