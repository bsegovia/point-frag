#!/bin/bash

files=`find . -name '*pp'`
for f in $files; do
		cp $f $f.bak
		cat $f.bak | sed "s/Renderer/RendererDriver/g" > $f
done

