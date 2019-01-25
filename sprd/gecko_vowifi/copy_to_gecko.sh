#!/bin/bash

basename=$(basename $0)
realpath=$(readlink -f $0)
newdir=$(dirname $realpath)
relativepath=$(readlink -e $newdir/../../../)

src=$relativepath/vendor/sprd/gecko_vowifi
dest=$relativepath/gecko/dom

echo "source dir is $src !"
echo "dest dir is $dest !"

ln -sf $src/imsservice/ $dest
ln -sf $src/vowifi/ $dest
