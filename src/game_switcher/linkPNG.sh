#!/bin/sh

#find -name "libpng.so"
#tout enlever !

#rm /usr/lib/libpng.so
rm /usr/lib/arm-linux-gnueabihf/libpng.so

ln -s /usr/lib/libpng12.so /usr/lib/arm-linux-gnueabihf/libpng.so

rm /usr/include/png.h
rm /usr/include/pngconf.h

cp /root/workspace/src/MainUiBatPerc/png.h /usr/include/png.h
cp /root/workspace/src/MainUiBatPerc/pngconf.h /usr/include/pngconf.h



