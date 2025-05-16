palette-opt
===========

This is a utility used for calculating color mixing weights in STDOOM.
It assumes a hard-coded 256 color palette (DOOM's PLAYPAL) and a hard-coded subset of 16 of these colors.
It then performs an exhaustive search for ho to best mix the 256 original colors using a weighted sum of the 16 subset-colors.
It does so according to a hard-coded distance metric that takes into account

- the human eye's higher sensitivity to brightness variation than to color variation
- the sum of distances of each chosen subset-color to the target color, thereby keeping the dithering somewhat limited.

How to compile
==============
Just type `make`

How to run
==========
Just type `./palette-opt`

