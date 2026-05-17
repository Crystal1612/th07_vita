import os
import shutil
import subprocess
import sys
from pathlib import Path, PureWindowsPath
from typing import Dict, Iterator
from zipfile import ZipFile

import requests

DX8_URL = "https://archive.org/download/dx8sdk/dx8sdk.exe"
MSVC_URL = "https://archive.org/download/en_vs.net_pro_full/en_vs.net_pro_full.exe"
HACKERY_URL = "https://gist.githubusercontent.com/EstexNT/e98a1384b906a3eedaaa3eeb7e58cd9d/raw/822536a26025f0df8763f1112d89bb1514f6209c/hackery.cpp"
DX8_SIZE = 144441256
MSVC_SIZE = 1706945024


def conv_path(path: Path) -> str:
    """Convert a Unix path to a Windows path, if needed."""
    if sys.platform == "win32":
        return str(PureWindowsPath(path))

    return "Z:" + str(path.resolve())


def run_program(name: str, *args: str, env: Dict[str, str]):
    if sys.platform == "win32":
        cmd = [name] + list(args)
    else:
        env["LANG"] = "ja_JP.UTF-8"
        env["WINEDEBUG"] = "fixme-all"
        cmd = ["wine", name] + list(args)
    return subprocess.check_call(cmd, env=env)


# Oh My God Bruh
def fixup_msiextract(path: Path):
    # msiextract makes absolutely no attempt at resolving the mappings and renaming.
    # this is done to extract the product name from the files (the first part of it)
    # since we dont really care about the rest.
    # do files first to avoid issues with renaming directories before files
    for file in path.rglob("*"):
        if not file.is_dir():
            name = file.name
            if ":" in name and "|" in name:
                name = name.split("|")[0]
                name = name.split(":")[0]
                _ = file.rename(file.with_name(name))

    dirs = [p for p in path.rglob("*") if p.is_dir()]

    dirs.sort(key=lambda p: len(p.parts), reverse=True)
    for dir in dirs:
        name = dir.name
        if ":" in name and "|" in name:
            name = name.split("|")[0]
            name = name.split(":")[0]
            name = name.upper()
            if not dir.with_name(name).exists():
                _ = dir.rename(dir.with_name(name))
        # this is done specifically for the folders in platformsdk.
        # just get the part out that looks like a directory name
        elif name.startswith(".:"):
            name = name.split(":")[1]
            name = name.upper()
            if not dir.with_name(name).exists():
                _ = dir.rename(dir.with_name(name))
            else:
                _ = shutil.copytree(dir, dir.with_name(name), dirs_exist_ok=True)


def download(url: str, dest_path: Path):
    response = requests.get(url, stream=True)
    total = int(response.headers.get("content-length", 0))
    if response.status_code == 200:
        with open(dest_path, "wb") as file:
            downloaded = 0
            chunk_iter: Iterator[bytes] = response.iter_content(chunk_size=1024 * 1024)
            for data in chunk_iter:
                downloaded += file.write(data)
                percent = (downloaded / total) * 100
                print(f"\rDownloading {url} ... {percent:.2f}%", end="")
    else:
        raise Exception(f"Failed to download {url}")


def download_dx8(path: Path):
    if not path.exists():
        os.makedirs(path)
    if (path / "include").exists():
        return
    archive_path = path / "dx8sdk.exe"
    if not os.path.exists(archive_path) or archive_path.stat().st_size != DX8_SIZE:
        download(DX8_URL, archive_path)

    # these happen to be valid zips, too
    with ZipFile(archive_path, "r") as zip:
        zip.extractall(path)


# this is awful
def download_msvc(path: Path, vs_path: Path, vc_path: Path):
    if not path.exists():
        os.makedirs(path)
    if (vc_path / "BIN" / "CL.EXE").exists():
        return
    archive_path = path / "en_vs.net_pro_full.exe"
    if not os.path.exists(archive_path) or archive_path.stat().st_size != MSVC_SIZE:
        download(MSVC_URL, archive_path)
    with ZipFile(archive_path, "r") as zip:
        _ = zip.extractall(path)
    if sys.platform == "win32":
        _ = subprocess.check_call(
            [
                "msiexec",
                "/a",
                str(path / "VS_SETUP.MSI"),
                "/qb",
                f'TARGETDIR="{path}"',
            ]
        )
    else:
        _ = subprocess.check_call(["msiextract", "-C", path, path / "VS_SETUP.MSI"])
        fixup_msiextract(path)
        # lets just assume you're on a case sensitive filesystem
        _ = shutil.copytree(
            path / "Program Files", path / "PROGRAM FILES", dirs_exist_ok=True
        )
        _ = shutil.rmtree(path / "Program Files")

    # we dont really need anything that isnt already inside of program files
    for file in path.iterdir():
        if (
            file.name.upper() == "PROGRAM FILES"
            or file.name == "en_vs.net_pro_full.exe"
        ):
            continue
        if file.is_file():
            os.remove(file)
        elif file.is_dir():
            shutil.rmtree(file)
    # cl dll dependencies
    _ = (vs_path / "COMMON7" / "IDE" / "MSPDB70.DLL").rename(
        vc_path / "BIN" / "MSPDB70.DLL"
    )
    _ = (vs_path / "COMMON7" / "IDE" / "MSOBJ10.DLL").rename(
        vc_path / "BIN" / "MSOBJ10.DLL"
    )


# provides pragma var_order
def install_hackery(cl_path: Path, msvc_path: Path, vc_path: Path, env: Dict[str, str]):
    c1xx_path = vc_path / "BIN" / "C1XX.DLL"
    orig_path = vc_path / "BIN" / "C1XXOrig.dll"

    if orig_path.exists() and c1xx_path.exists():
        return
    os.chdir(msvc_path)
    download(HACKERY_URL, msvc_path / "hackery.cpp")
    if not orig_path.exists():
        _ = shutil.copy(c1xx_path, orig_path)
    _ = run_program(
        str(cl_path),
        "/LD",
        conv_path(msvc_path / "hackery.cpp"),
        "/link",
        f"/OUT:{msvc_path / 'C1XX.DLL'}",
        env=env,
    )
    _ = (msvc_path / "C1XX.DLL").replace(c1xx_path)
