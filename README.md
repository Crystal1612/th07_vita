# th07

<img src="resources/progress.svg" alt="Implemented: 100%. Accuracy: 98.54%" width="50%">

A work-in-progress reimplementation/decompilation of 東方妖々夢　～ Perfect Cherry Blossom 1.00b (md5: 0126afce1e805370d36c3482445e98da) by Team Shanghai Alice.

The game is "playable," as in it is 100% implemented, and you can load into the game and play from start to finish, and the behavior of the program _should_ be mostly identical to the original, but there may be a number of bugs since the game isn't yet fully matching. Perfect byte accuracy is an eventual goal.

## Building

This project requires the original th07.exe 1.00b executable for extracting the icon. Copy it to the resources directory of the repository.

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

This executable _will_ crash after some time (specifically after 3999 Supervisor cycles). It'll fail the integrity check due to the executable not (yet?) being completely byte accurate (including checksum) to the original. In that case, you can try building a nonmatching build instead, which will disable this integrity check:

```
uv run scripts/build.py --no-matching
```

If you don't have the original executable, you can still build the program without the icon.

```
uv run scripts/build.py --no-icon
```

## Todo

* Clean up this complete mess of code.
* Start matching (and fixing issues).
* Get a better build system than whatever this is

## Contributing

See the [CONTRIBUTING.md](./CONTRIBUTING.md).

## Credits

* The earlier [decompilation for th06](https://github.com/GensokyoClub), used as a source of shared types, function names, file names, source organization, basically everything. Because EoSD and PCB are so similar architecturally, the pre-existing th06 decompilation could be used as a direct reference for reverse engineering th07.

* EstexNT for porting the [var_order pragma](https://gist.github.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d) to MSVC 7, which is used extensively throughout this project.
