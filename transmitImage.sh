#!/bin/bash

#   Copyright 2013 JP Meijers
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

while true; do

	python AircamDownload.py

	timestamp=`date '+%F %R'`

	convert AircamImage.jpg -resize 320x256\! resized.jpg
	convert -pointsize 40 -fill yellow -stroke black -draw "text 6,250 ZS1JPM" resized.jpg overlayed.jpg
	convert -pointsize 30 -fill yellow -stroke black -draw "text 6,32 '$timestamp'" overlayed.jpg overlayed.jpg
	convert -pointsize 30 -fill yellow -stroke black -draw "rotate -90 text -250,310 'PiController'" overlayed.jpg overlayed.jpg

	./sstvtx -i overlayed.jpg -o toTX.wav

	aplay 1200tone.wav
	aplay toTX.wav

	sleep 30m

done;
