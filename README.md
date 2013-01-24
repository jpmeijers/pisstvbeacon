Raspberry Pi SSTV beacon
========================

This folder contains a few scripts and a program to:
1. Capture an image from an Ubiquity IP camera (AircamDownload.py)
2. Converts the jpeg image to a wav file, Martin1 encoded (sstvtx.c)
3. Automate the capture, resize, overlay, encode and playback of the wav file (transmitImage.sh)


Required programs
=================

* _Python 2.7_ to execute AircamDownload.py
* _Imagemagick_ convert to resize and overlay the image
* _aplay_ to play back the wav files


How to build
============

Except for sstvtx.c, the rest of this folder only contains scripts that can be run without compilation. To compile sstvtx.c on a Raspberry Pi, I used the following command:
    gcc sstvtx.c -o sstvtx -ljpeg -lsndfile
    

License
=======

* _sstvtx.c_ is released under the Mit License according to the source: http://fkurz.net/ham/stuff.html?sstvtx
* _AircamDownload.py_ originally found on http://www.exploit-db.com/ without a license.
* The rest of the contents of this directory/folder/project is released under Apache2.0    
