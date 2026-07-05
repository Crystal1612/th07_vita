import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List

from download_deps import conv_path, download_dx8, download_msvc

SCRIPT_PATH = Path(os.path.abspath(__file__))
PROJ_DIR = Path(os.path.dirname(SCRIPT_PATH)).parent

os.chdir(PROJ_DIR)

SRC_DIR = PROJ_DIR / "src"
BUILD_DIR = PROJ_DIR / "build"
MSVC_PATH = PROJ_DIR / "thirdparty" / "msvc"
DX8_PATH = PROJ_DIR / "thirdparty" / "dx8"
BUILD_PATH = BUILD_DIR / "th07.exe"

VS_PATH = MSVC_PATH / "Program Files" / "Microsoft Visual Studio .NET"
VC_PATH = VS_PATH / "Vc7"
CL_PATH = VC_PATH / "bin" / "cl.exe"
LINK_PATH = VC_PATH / "bin" / "link.exe"
RC_PATH = VC_PATH / "bin" / "rc.exe"

SOURCES: List[Path] = list(
    map(
        lambda x: Path(x),
        [
            "AnmVm.cpp",
            "AsciiManager.cpp",
            "Stage.cpp",
            "BombData.cpp",
            "EclManager.cpp",
            "EnemyEclInstr.cpp",
            "EffectManager.cpp",
            "Ending.cpp",
            "EnemyManager.cpp",
            "BulletManager.cpp",
            "Gui.cpp",
            "GameManager.cpp",
            "Chain.cpp",
            "Controller.cpp",
            "FileSystem.cpp",
            "GameErrorContext.cpp",
            "Rng.cpp",
            "utils.cpp",
            "TextHelper.cpp",
            "ItemManager.cpp",
            "main.cpp",
            "GameWindow.cpp",
            "MidiOutput.cpp",
            "Supervisor.cpp", # ZUN name: mother.cpp
            "MusicRoom.cpp",
            "Player.cpp",
            "ReplayManager.cpp",
            "ResultScreen.cpp",
            "ScreenEffect.cpp",
            "SoundPlayer.cpp",
            "AnmManager.cpp",
            "MainMenu.cpp",
            "dsutil.cpp",
            "pbg4/Pbg4File.cpp",
            "pbg4/Lzss.cpp",
            "pbg4/Pbg4Archive.cpp",
        ],
    )
)

SMALL_SOURCES: List[Path] = list(
    map(
        lambda x: Path(x),
        [
            "GameManager.cpp",
            "Gui.cpp",
            "MainMenu.cpp",
            "MusicRoom.cpp",
            "ResultScreen.cpp",
            "Supervisor.cpp",
            "TextHelper.cpp",
        ],
    )
)

SMALL_NON_INTRINSIC_SOURCES: List[Path] = list(
    map(
        lambda x: Path(x),
        [
            "MainMenu.cpp",
            "ResultScreen.cpp",
            "TextHelper.cpp",
        ]
    )
)

if not shutil.which("ninja"):
    sys.exit("ninja must be installed when building")

download_dx8(DX8_PATH)
download_msvc(MSVC_PATH, VS_PATH, VC_PATH)

cflags = [
    "/nologo",
    "/W3",
    "/MT",
    "/Ob1",
    "/EHsc",
    "/Gr",
    "/GL",
    "/Gy",
    "/GF",
    "/Zi",
    "/DNDEBUG",
    "/wd4060",
    "/wd4101", # these just got really annoying
    "/wd4244",
    f'/I"{conv_path(VC_PATH / "include")}"',
    f'/I"{conv_path(VC_PATH / "PlatformSDK" / "include")}"',
    f'/I"{conv_path(DX8_PATH / "include")}"',
]

lflags = [
    f'/LIBPATH:"{conv_path(VC_PATH / "lib")}"',
    f'/LIBPATH:"{conv_path(VC_PATH / "PlatformSDK" / "lib")}"',
    f'/LIBPATH:"{conv_path(DX8_PATH / "lib")}"',
    "/LTCG",
    "/INCREMENTAL:NO",
    "/MAP",
    "/OPT:REF",
    "/OPT:ICF",
    "/DEBUG",
]

libs = [
    "dinput8.lib",
    "dsound.lib",
    "d3d8.lib",
    "d3dx8.lib",
    "dxguid.lib",
    "gdi32.lib",
    "user32.lib",
    "winmm.lib",
    "ole32.lib",
]

os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

with open("build.ninja", "w") as f:
    f.write("ninja_required_version = 1.3\n\n")

    if sys.platform != "win32":
        wine_cmd = "env LANG=ja_JP.UTF-8 WINEDEBUG=fixme-all wine "
        f.write(
            f'cl = "{sys.executable}" ../scripts/cl_wrapper.py "{conv_path(CL_PATH)}"\n'
        )
    else:
        wine_cmd = ""
        f.write(f'cl = "{conv_path(CL_PATH)}"\n')

    f.write(f'link = {wine_cmd}"{conv_path(LINK_PATH)}"\n')
    f.write(f'rc = {wine_cmd}"{conv_path(RC_PATH)}"\n')

    f.write(f"cflags = {'/Od /Oi ' + ' '.join(cflags)}\n")
    f.write(f"cflags_small = {'/Os /Oi ' + ' '.join(cflags)}\n")
    f.write(f"cflags_small_nonintrinsic = {'/Os ' + ' '.join(cflags)}\n")
    f.write(f"lflags = {' '.join(lflags)}\n")
    f.write(f"libs = {' '.join(libs)}\n\n")

    f.write("rule cxx\n")
    f.write("  command = $cl /showIncludes $in_cflags /c $in /Fo$out /Fd$pdb\n")
    f.write("  description = cxx $out\n")
    f.write("  deps = msvc\n\n")

    f.write("rule link\n")
    f.write("  command = $link $in $lflags $libs /OUT:$out\n")
    f.write("  description = link $out\n\n")

    objects = []
    for src in SOURCES:
        src_name = src.as_posix()
        obj_name = "obj/" + src.with_suffix(".obj").as_posix()
        pdb_name = "obj/" + src.with_suffix(".pdb").as_posix()
        f.write(f"build {obj_name}: cxx ../src/{src_name}\n")
        if src not in SMALL_SOURCES:
            f.write("  in_cflags = $cflags\n")
        elif src in SMALL_NON_INTRINSIC_SOURCES:
            f.write("  in_cflags = $cflags_small_nonintrinsic\n")
        else:
            f.write("  in_cflags = $cflags_small\n")
        f.write(f"  pdb = {pdb_name}\n")
        objects.append(obj_name)
    f.write("\n")

    f.write(f"build th07.exe: link {' '.join(objects)}\n")

_ = subprocess.check_call(["ninja"])
