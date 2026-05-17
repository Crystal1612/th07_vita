import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List

from download_deps import conv_path, download_dx8, download_msvc, install_hackery

SCRIPT_PATH = Path(os.path.abspath(__file__))
PROJ_DIR = Path(os.path.dirname(SCRIPT_PATH)).parent

os.chdir(PROJ_DIR)

SRC_DIR = PROJ_DIR / "src"
BUILD_DIR = PROJ_DIR / "build"
RESOURCE_DIR = PROJ_DIR / "resources"
MSVC_PATH = PROJ_DIR / "thirdparty" / "msvc"
DX8_PATH = PROJ_DIR / "thirdparty" / "dx8"
EXE_PATH = RESOURCE_DIR / "th07.exe"
BUILD_PATH = BUILD_DIR / "th07.exe"

VS_PATH = MSVC_PATH / "PROGRAM FILES" / "MICROSOFT VISUAL STUDIO .NET"
VC_PATH = VS_PATH / "VC7"
CL_PATH = VC_PATH / "BIN" / "CL.EXE"
LINK_PATH = VC_PATH / "BIN" / "LINK.EXE"
RC_PATH = VC_PATH / "BIN" / "RC.EXE"

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
            "Supervisor.cpp",
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

parser = argparse.ArgumentParser()
_ = parser.add_argument(
    "--no-icon",
    action="store_true",
    help="build without requiring an icon from the original executable",
)
_ = parser.add_argument(
    "--no-matching", action="store_true", help="build without attempting matching"
)
subparsers = parser.add_subparsers(dest="command")

parser_reccmp = subparsers.add_parser("reccmp", help="output reccmp output")
parser_stackcmp = subparsers.add_parser(
    "stackcmp", help="compare stack layout with stackcmp"
)
parser_datacmp = subparsers.add_parser("datacmp", help="compare globals with datacmp")
parser_roadmap = subparsers.add_parser(
    "roadmap", help="compare symbol locations with roadmap"
)

_ = parser_reccmp.add_argument(
    "address",
    nargs="?",
    default=None,
    help="optional function address for displaying diff",
)
_ = parser_reccmp.add_argument(
    "--init",
    action="store_true",
    help="initialize reccmp project",
)
_ = parser_reccmp.add_argument(
    "--svg", action="store_true", help="generate progress svg"
)
_ = parser_stackcmp.add_argument(
    "address", help="function address for displaying stack layout"
)
args = parser.parse_args()


def extract_icon(path: Path) -> Path:
    """Extract the icon from a PE file located at path"""
    ico_path = BUILD_DIR / path.with_suffix(".ico").name

    # icoextract doesn't have an actual scripting API, so it has to be shelled out to instead.
    # should be fine since you're running this through uv anyways which puts the venv in PATH
    cmd = ["icoextract", str(path), ico_path]
    _ = subprocess.check_call(cmd)
    return ico_path


if not EXE_PATH.exists():
    if args.command:
        sys.exit("th07.exe should exist when using any reccmp tools")
    elif not args.no_icon:
        sys.exit("th07.exe should exist when building with icon")

if not shutil.which("ninja"):
    sys.exit("ninja must be installed when building")

env = os.environ.copy()

download_dx8(DX8_PATH)
download_msvc(MSVC_PATH, VS_PATH, VC_PATH)
install_hackery(CL_PATH, MSVC_PATH, VC_PATH, env)

cflags = [
    "/nologo",
    "/W3",
    "/MT",
    "/Od",
    "/Ob1",
    "/Oi",
    "/EHsc",
    "/Gr",
    "/GL",
    "/Gy",
    "/GF",
    "/Zi",
    "/DNDEBUG",
    f'/I"{conv_path(VC_PATH / "INCLUDE")}"',
    f'/I"{conv_path(VC_PATH / "PLATFORMSDK" / "COMMON" / "Include")}"',
    f'/I"{conv_path(DX8_PATH / "include")}"',
]

if args.no_matching:
    cflags.append("/DNON_MATCHING")

lflags = [
    f'/LIBPATH:"{conv_path(VC_PATH / "LIB")}"',
    f'/LIBPATH:"{conv_path(VC_PATH / "PLATFORMSDK" / "COMMON" / "lib")}"',
    f'/LIBPATH:"{conv_path(DX8_PATH / "lib")}"',
    "/LTCG",
    "/INCREMENTAL:NO",
    "/MAP",
    "/OPT:REF",
    "/OPT:NOICF",
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

    f.write(f"cflags = {' '.join(cflags)}\n")
    f.write(f"lflags = {' '.join(lflags)}\n")
    f.write(f"libs = {' '.join(libs)}\n\n")

    f.write("rule cxx\n")
    f.write("  command = $cl /showIncludes $cflags /c $in /Fo$out /Fd$pdb\n")
    f.write("  description = cxx $out\n")
    f.write("  deps = msvc\n\n")

    f.write("rule rc\n")
    f.write("  command = $rc /fo$out $in\n")
    f.write("  description = rc $out\n\n")

    f.write("rule link\n")
    f.write("  command = $link $in $lflags $libs /order:@$order /OUT:$out\n")
    f.write("  description = link $out\n\n")

    objects = []
    for src in SOURCES:
        src_name = src.as_posix()
        obj_name = "obj/" + src.with_suffix(".obj").as_posix()
        pdb_name = "obj/" + src.with_suffix(".pdb").as_posix()
        f.write(f"build {obj_name}: cxx ../src/{src_name}\n")
        f.write(f"  pdb = {pdb_name}\n")
        objects.append(obj_name)
    f.write("\n")

    if not args.no_icon:
        icon_path = extract_icon(EXE_PATH)
        with open("resources.rc", "w") as g:
            _ = g.write(f'1 ICON "{icon_path.name}"\n')
        f.write("build resources.res: rc resources.rc\n\n")
        objects.append("resources.res")

    f.write(f"build th07.exe: link {' '.join(objects)}\n")
    f.write(f"  order = {conv_path(RESOURCE_DIR / 'order.txt')}\n")

_ = subprocess.check_call(["ninja"])

match args.command:
    case "reccmp":
        if args.init:
            os.chdir(PROJ_DIR)
            _ = subprocess.check_call(
                ["reccmp-project", "detect", "--search-path", RESOURCE_DIR]
            )
            os.chdir(BUILD_DIR)
            _ = subprocess.check_call(
                ["reccmp-project", "detect", "--what", "recompiled"]
            )
        elif args.svg:
            _ = subprocess.check_call(
                [
                    "reccmp-reccmp",
                    "--target",
                    "TH07",
                    "--svg",
                    RESOURCE_DIR / "progress.svg",
                    "--svg-icon",
                    RESOURCE_DIR / "svgicon.png",
                    "--nolib",
                ]
            )
        elif args.address:
            _ = subprocess.check_call(
                [
                    "reccmp-reccmp",
                    "--target",
                    "TH07",
                    "--html",
                    "index.html",
                    "--nolib",
                    "--verbose",
                    args.address,
                ]
            )
        else:
            _ = subprocess.check_call(
                ["reccmp-reccmp", "--target", "TH07", "--html", "index.html", "--nolib"]
            )
    case "stackcmp":
        _ = subprocess.call(
            [
                "reccmp-stackcmp",
                "--target",
                "TH07",
                args.address,
            ]
        )
    case "datacmp":
        _ = subprocess.call(
            [
                "reccmp-datacmp",
                "--target",
                "TH07",
            ]
        )
    case "roadmap":
        _ = subprocess.call(
            [
                "reccmp-roadmap",
                "--target",
                "TH07",
            ]
        )
    case _:
        pass
