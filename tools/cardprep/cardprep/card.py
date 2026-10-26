"""Checks that a path lives on a FAT32 volume of at most 32 GB."""
import shutil
import subprocess
import sys
from pathlib import Path

MAX_CARD_BYTES = 32 * 1024**3
_MACOS_FAT32 = {"MS-DOS", "MS-DOS FAT32"}


class CardError(Exception):
    pass


def _run(cmd: list[str]) -> str:
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, stdin=subprocess.DEVNULL)
    except OSError as e:
        raise CardError(f"could not run {cmd[0]}: {e}") from e
    if r.returncode != 0:
        raise CardError(f"{' '.join(cmd)} failed: {r.stderr.strip() or r.stdout.strip()}")
    return r.stdout


def _macos_mount_point(path: Path) -> str:
    lines = _run(["df", "-P", str(path)]).splitlines()
    if len(lines) < 2 or not lines[1].split():
        raise CardError(f"unexpected df output for {path}")
    # The mount point is the last column but may contain spaces; df -P has 5 fixed
    # columns before it.
    parts = lines[1].split(None, 5)
    if len(parts) < 6:
        raise CardError(f"unexpected df output for {path}")
    return parts[5]


def _macos_personality(mount: str) -> str:
    for line in _run(["diskutil", "info", mount]).splitlines():
        key, _, value = line.partition(":")
        if key.strip() == "File System Personality":
            return value.strip()
    raise CardError(f"diskutil did not report a file system for {mount}")


def _linux_fstype(path: Path) -> str:
    fstype = _run(["findmnt", "-no", "FSTYPE", "--target", str(path)]).strip()
    if not fstype:
        raise CardError(f"findmnt did not report a file system for {path}")
    return fstype.splitlines()[0].strip()


def check_card(path: Path) -> None:
    """Raise CardError unless `path` is on a FAT32 volume of at most 32 GB."""
    path = Path(path)
    if not path.exists():
        raise CardError(f"{path} does not exist")
    if sys.platform == "darwin":
        mount = _macos_mount_point(path)
        personality = _macos_personality(mount)
        if personality not in _MACOS_FAT32:
            raise CardError(f"{mount} is {personality}, not FAT32; reformat the card as FAT32")
    else:
        fstype = _linux_fstype(path)
        if fstype != "vfat":
            raise CardError(f"{path} is on {fstype}, not FAT32 (vfat); reformat the card as FAT32")
        mount = str(path)
    try:
        total = shutil.disk_usage(mount).total
    except OSError as e:
        raise CardError(f"could not read the size of {mount}: {e}") from e
    if total > MAX_CARD_BYTES:
        raise CardError(f"{mount} is {total / 1024**3:.0f} GB; cards over 32 GB are not supported")
