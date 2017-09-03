#! /bin/sh
$EXTRACTRC `find . -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp` `find ../libdialogutil -name \*.cpp` -o $podir/libkookascan.pot
