# th07_vita

A cross-platform port of 東方妖々夢　～ Perfect Cherry Blossom 1.00b by Team Shanghai Alice using SDL2 and OpenGL ES.

This is the PSVita branch of the Touhou 7 decompilation. Unless you're looking specifically for an attempted cross-platform port of th07, you probably want the [main branch](https://github.com/some100/th07/tree/main). 


### Dependencies

* cmake
* vitasdk
* [SDL2](https://github.com/libsdl-org/SDL/tree/SDL2) (SDL2, SDL2_ttf, and SDL2_image) 
Tested only on"-DVIDEO_VITA_PIB=ON"


### Play
* Install PSM Runtime
[CrystalPSM](https://github.com/EliCrystal2001/CrystalPSM)
[PIB-Configuration-Tool](https://github.com/SonicMastr/PIB-Configuration-Tool/tree/main)
* Install VPK
* Copy original game files to: ux0:data/th07
* Copy "msgothic.ttc" font to ux0:data/th07


## Credits
* portable branch authored by [some100](https://github.com/some100/th07/tree/portable). 

* The earlier [decompilation for th06](https://github.com/GensokyoClub/th06), used as a source of shared types, function names, file names, source organization, basically everything. Because EoSD and PCB are so similar architecturally, the pre-existing th06 decompilation could be used as a direct reference for reverse engineering th07.

* The [decompilation for th08](https://github.com/GensokyoClub/th08) for the complete and actually readable LZSS implementation. Basically nothing changed from th07 to th08 at least in this regard, so it made it much simpler.

* EstexNT for porting the [var_order pragma](https://gist.github.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d) to MSVC 7, which is used extensively throughout this project.
