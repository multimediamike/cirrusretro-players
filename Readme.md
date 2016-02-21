# Cirrus Retro Players

## About

This repository is a collection of C/C++ players for game music file formats
that can be compiled to JavaScript using
[Emscripten](http://kripken.github.io/emscripten-site/) and run in a variety
of modern web browsers, including Google Chrome, Mozilla Firefox, Apple Safari,
and Microsoft Edge.

## Supported file formats:

* AY, either raw or compressed with xz-crc32
* GBS, either raw or compressed with xz-crc32
* GYM, either raw or compressed with xz-crc32
* HES, either raw or compressed with xz-crc32
* KSS, either raw or compressed with xz-crc32
* NSF, either raw or compressed with xz-crc32
* SAP, either raw or compressed with xz-crc32
* SPC, either raw or packaged within a .gamemusic archive, and can be compressed with xz-crc32; note that .rsn format (RAR), the prevailing method for packaging SPC files, is *not* supported
* VGM, either raw or packaged within a .gamemusic archive, and can be compressed with xz-crc32
* 2SF Nintendo DS sound files, but only if they are packaged as .psfarchive files
* SSF Sega Saturn sound files, but only if they are packaged as .psfarchive files
* DSF Sega Dreamcast sound files, but only if they are packaged as .psfarchive files

## Building
To build all the players, type `make`. Note that this requires that Emscripten
be installed as well as [ccache](https://ccache.samba.org/) for accelerating successive builds.

Clean intermediate files using `make clean`.

Note that the build script uses the '-j' option which does not limit the
number of parallel build jobs. Specify the environment variable
`BUILD_THREADS` in order to limit the number of jobs. E.g.:
`BUILD_THREADS=2 make` will only build using 2 processes.

## Demonstration
After building the players, you can run the included demo web server to
see the files in action. Run './cr-test-server.py' and point a web browser at
the server. You will be able to select a type of music files to play and test
out the various player features, including visualizations, channel toggling,
track advancement, playback time reporting, volume control, and pause/resume.

## Testing
Run the script './run-tests.py' to run the integration tests and
validate that all the players are working correctly. This requires
that Node.js be installed.

## Usage
JavaScript that wants to leverage the infrastructure will load the file
play.js and use the API contained within.
