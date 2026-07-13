# th07

A cross-platform port of 東方妖々夢　～ Perfect Cherry Blossom 1.00b by Team Shanghai Alice.

This is the portable branch of the Touhou 7 decompilation. Unless you're looking specifically for an attempted cross-platform port of th07, you probably want the [main branch](https://github.com/some100/th07/tree/main). Currently, this will not produce a playable game on any platform, and is broken and unfinished right now. Rendering seems to work fine, as does sound, but there are things that don't work fine. Namely:

* You cannot load into stages (on 64-bit). This is because the way ecl files, stg files, etc. are loaded in the original game is not endian or alignment independent, resulting in it breaking on any system not on 32-bit little endian.
* Text rendering looks off. To be clear it does "work" but the text looks too big.
* There is only a software renderer implemented, which is ridiculously slow particularly on debug builds.
* Some features that the original game had, like 16 bit color mode, midi output, etc. are outright unimplemented. This may or may not be "fixed" later, but the focus currently is to produce a playable game.

Work is currently being done to transition the game over to being more platform-independent.

## Building

### Dependencies

* cmake
* SDL2
* A compiler that supports C++17

Run cmake on this repo, then build with whatever generator you chose.

You may also need to add a copy of `msgothic.ttc` into your game directory if you are not running this on Windows.

## Credits

* The earlier [decompilation for th06](https://github.com/GensokyoClub/th06), used as a source of shared types, function names, file names, source organization, basically everything. Because EoSD and PCB are so similar architecturally, the pre-existing th06 decompilation could be used as a direct reference for reverse engineering th07.

* The [decompilation for th08](https://github.com/GensokyoClub/th08) for the complete and actually readable LZSS implementation. Basically nothing changed from th07 to th08 at least in this regard, so it made it much simpler.

* EstexNT for porting the [var_order pragma](https://gist.github.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d) to MSVC 7, which is used extensively throughout this project.
