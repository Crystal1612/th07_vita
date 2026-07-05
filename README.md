# th07

A cross-platform port of 東方妖々夢　～ Perfect Cherry Blossom 1.00b by Team Shanghai Alice.

This is the portable branch of the Touhou 7 decompilation. Currently, this will not produce a playable game on any platform other than Windows. Work is currently being done to transition the game over to being more platform-independent.

## Building

### Dependencies

* uv
* ninja
* wine (Linux only)
    * Note: extracting the MSVC msi is completely broken on older versions of wine. If you face an issue with extracting, try using the latest devel version of wine.

Run the python script in the root directory of the repo with uv:

```
uv run scripts/build.py
```

The resulting build can be found at `build/th07.exe`.

## Contributing

See the [CONTRIBUTING.md](./CONTRIBUTING.md).

## Credits

* The earlier [decompilation for th06](https://github.com/GensokyoClub/th06), used as a source of shared types, function names, file names, source organization, basically everything. Because EoSD and PCB are so similar architecturally, the pre-existing th06 decompilation could be used as a direct reference for reverse engineering th07.

* The [decompilation for th08](https://github.com/GensokyoClub/th08) for the complete and actually readable LZSS implementation. Basically nothing changed from th07 to th08 at least in this regard, so it made it much simpler.

* EstexNT for porting the [var_order pragma](https://gist.github.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d) to MSVC 7, which is used extensively throughout this project.
