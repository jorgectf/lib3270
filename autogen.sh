#!/bin/bash

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

cd "$srcdir"

mkdir -p scripts
mkdir -p m4

libtoolize --force
if test $? != 0 ; then
	echo "libtoolize failed."
	exit -1
fi

aclocal
if test $? != 0 ; then
	echo "aclocal failed."
	exit -1
fi

#autoheader --force
#if test $? != 0 ; then
#	echo "autoheader failed."
#	exit -1
#fi

autoconf --force
if test $? != 0 ; then
	echo "autoconf failed."
	exit -1
fi

automake --add-missing 2> /dev/null | true

autopoint

test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"




