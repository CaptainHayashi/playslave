playslave
=========

*playslave* is an attempt at making a small audio playback wrapper on top of *ffmpeg* and *portaudio* that can be controlled through stdout and stdin with a simple protocol.

It might end up being the bottom rung of a playback system someday.  Who knows.

It is written in C99 against POSIX.1-2008, and should compile on any recent nix-like.

Usage
-----

More functionality to be added later.

- Command argument is the libao ID to output to.
- *playslave* starts in the **EJECTED** state.
- `LOAD path/to/file` - loads file, stops any current playback, places *playslave* in **STOPPED** state.
- `PLAY` - plays file when in **STOPPED** state, moves *playslave* to **PLAYING** state.
- `EJCT` - ejects file when in **STOPPED** or **PLAYING** state.
- `QUIT` - kills *playslave*.

Known issues
------------

- STOPping currently drops a frame or two on resumption
- No way to seek
- No real use as of yet

Acknowledgments
---------------

A lot of the audio bits of this were salvaged from example code, most notably
http://blinkingblip.wordpress.com/2011/10/08/decoding-and-playing-an-audio-stream-using-libavcodec-libavformat-and-libao/
and some other sources.

Licence
-------

*playslave* is distributed under the terms of the GNU General Public License, version 2.

*ffmpeg* and *portaudio* are used by *playslave*; they are also available under the terms of the GPLv2 and MIT licences respectively.
