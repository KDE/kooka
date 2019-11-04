#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp` `find ../plugins -name \*.cpp` -o $podir/kooka.pot
rm -f rc.cpp
