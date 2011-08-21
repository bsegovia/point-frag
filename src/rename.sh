#!/bin/bash

files=`find . -name '*pp'`
for f in $files; do
		cp $f $f.bak
		cat $f.bak | sed "s/ALIGNEDed__/ALIGNED/g" > $f
done

