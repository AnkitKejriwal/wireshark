#!/bin/sh

DOWNLOAD_PREFIX="http://www.ethereal.com/distribution/win32/development"

err_exit () {
	echo "ERROR: $1"
	echo ""
	exit 1
}

usage () {
	echo "Usage:"
	echo "	$0 --appverify <appname> [<appname>] ..."
	echo "	$0 --download <destination> <subdirectory> <package>"
	echo ""
	exit 1
}


case "$1" in
'--appverify')
	shift
	if [ "$*" == "" ] ; then
		usage
	fi
	echo "Checking for required applications:"
	for APP in $* ; do
		APP_LOC=`which $APP 2> /dev/null`
		if [ "$APP_LOC" = "" ] ; then
			err_exit "Can't find $APP"
		fi
		echo "	$APP: $APP_LOC $res"
	done
	;;
'--download')
	if [ "$2" == "" -o "$3" == "" -o "$4" == "" ] ; then
		usage
	fi
	DEST_PATH=`cygpath --unix "$2"`
	DEST_SUBDIR=$3
	PACKAGE_PATH=$4
	PACKAGE=`basename "$PACKAGE_PATH"`
	echo "Downloading $4 into $DEST_PATH, installing into $3"
	if [ ! -d "$DEST_PATH/$DEST_SUBDIR" ] ; then
		mkdir -p "$DEST_PATH/$DEST_SUBDIR" || \
			err_exit "Can't create $DEST_PATH/$DEST_SUBDIR"
	fi
	cd "$DEST_PATH" || err_exit "Can't find $DEST_PATH"
	wget -nc "$DOWNLOAD_PREFIX/$PACKAGE_PATH" || \
		err_exit "Can't download $DOWNLOAD_PREFIX/$PACKAGE_PATH"
	cd $DEST_SUBDIR
	echo "Extracting $DEST_PATH/$PACKAGE into $DEST_PATH/$DEST_SUBDIR"
	unzip -nq "$DEST_PATH/$PACKAGE" || 
		err_exit "Couldn't unpack $DEST_PATH/$PACKAGE"
	;;
'*')
	usage
	;;
esac

exit 0
