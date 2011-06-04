#! /bin/sh
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$XGETTEXT *.cpp -o $podir/kooka.pot
rm -f rc.cpp
