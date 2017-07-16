#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.kcfg` >> rc.cpp
$XGETTEXT *.cpp -o $podir/kooka.pot
rm -f rc.cpp
