#!/bin/sh
##########################################################################
##									##
##  This script is part of Kooka, a KDE scanning/OCR application.	##
##									##
##  This file may be distributed and/or modified under the terms of	##
##  the GNU General Public License version 2, as published by the	##
##  Free Software Foundation and appearing in the file COPYING		##
##  included in the packaging of this file.				##
##									##
##  Author:  Jonathan Marten <jjm AT keelhaul DOT me DOT uk>		##
##									##
##########################################################################

SRC=$1							# CMAKE_CURRENT_SOURCE_DIR
BIN=$2							# CMAKE_CURRENT_BINARY_DIR
VER=$3							# application version
SVN=$4							# SVN command


if [ -d "$SRC/.svn" ]					# source is under SVN
then
	if [ -n "$SVN" ]				# SVN command available
	then
		SVNREV=`$SVN info $SRC 2>/dev/null | sed -n -e'/^Revision:/ { s/^[^:]*: *//;p;q }'`
		SVNDATE=`$SVN info $SRC 2>/dev/null | sed -n -e'/^Last Changed Date:/ { s/^[^:]*: *//;s/  *[-+][0-9].*$//;p;q }'`
	else						# SVN command not found
		SVNREV="Unknown"
		SVNDATE=
	fi
else							# source not under SVN
	SVNREV=
	SVNDATE=
fi


TMPFILE="$BIN/svnversion.h.tmp"				# temporary header file
{
	echo "#ifndef VERSION"
	echo "#define VERSION              \"${VER}\""
	echo "#endif"
	echo
	echo "#define SVN_REVISION_STRING  \"${SVNREV}\""
	echo "#define SVN_LAST_CHANGE      \"${SVNDATE}\""
	if [ -n "$SVNREV" ]
	then
		echo "#define SVN_HAVE_VERSION     1"
	else
		echo "#define SVN_HAVE_VERSION     0"
	fi
	echo
} >$TMPFILE


OUTFILE="$BIN/svnversion.h"				# the real header file
if [ ! -f $OUTFILE ]					# does not exist yet
then
	echo "Creating $OUTFILE..."
else							# already exists
	if cmp -s $TMPFILE $OUTFILE			# is it still current?
	then
		rm $TMPFILE				# yes, nothing to do
		exit 0
	else
		echo "Updating $OUTFILE..."
	fi
fi


mv $TMPFILE $OUTFILE					# update the header file
echo "Current SVN revision:  $SVNREV"
echo "Current SVN date:      $SVNDATE"

exit 0
