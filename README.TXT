This is STDOOM, a port of DOOM to the Atari ST.

SYSTEM REQUIREMENTS

You need an Atari ST with at least 4MB of RAM. Running on an 8MHz machine should work, but is
expected to run unplayably slow.

HOW TO PLAY

- Copy the STDOOM.TOS executable into a folder on your ST's (real or emulated) hard drive.
  You can download the STDOOM.TOS executable here if you don't want to compile it yourself:
  https://github.com/indyjo/STDOOM/blob/master/linuxdoom-1.10/atari/stdoom.tos
- Put a WAD file like DOOM1.WAD next to it.
- Start STDOOM.TOS.
- Use keyboard or mouse for control, just as in the original game.

FEATURES

- Runs DOOM in 16 colors with dithering.
- Controllable with keyboard and mouse.
- Zoomed mode for higher performance (activate by reducing window size using = key or menu).

NOT IMPLEMENTED

- Audio sample playback
- Music playback
- Network gameplay

HOW TO COMPILE

You need a C compiler and GNU Make. I recommend compiling with Thorsten Otto's Crossmint tools:
    https://www.tho-otto.de/crossmint.php

In order to compile, go into the "linuxdoom-1.10" directory, have CC point to m68k-atari-mint-gcc
and run "make".

