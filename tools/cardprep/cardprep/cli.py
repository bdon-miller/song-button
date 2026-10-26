"""cardprep SOURCE_DIR CARD_DIR [--skip-card-check]"""
import argparse
import os
import shutil
import sys
from pathlib import Path

from cardprep import audio
from cardprep.card import CardError, check_card
from cardprep.index import write_index
from cardprep.naming import TooManySongs, assign_names


def _warn(msg: str) -> None:
    print(f"warning: {msg}", file=sys.stderr)


def _is_within(child: Path, parent: Path) -> bool:
    # samefile sees through case-insensitive filesystems (macOS), which resolve() does not.
    for p in [child, *child.parents]:
        if p.exists() and parent.exists() and os.path.samefile(p, parent):
            return True
    return child == parent or parent in child.parents


def _clear_dir(folder: Path) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    for item in folder.iterdir():
        if item.is_dir() and not item.is_symlink():
            shutil.rmtree(item)
        else:
            item.unlink()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="cardprep", description="Prepare a microSD card of MP3s.")
    parser.add_argument("source_dir", type=Path)
    parser.add_argument("card_dir", type=Path)
    parser.add_argument("--skip-card-check", action="store_true",
                        help="do not verify the card is FAT32 and at most 32 GB")
    args = parser.parse_args(argv)
    try:
        return _run(args)
    except KeyboardInterrupt:
        print("interrupted; card has no index, re-run to finish", file=sys.stderr)
        return 130
    except Exception as e:  # includes OSError; odd ffprobe output must not show a traceback
        print(f"error: {e}", file=sys.stderr)
        return 2


def _run(args: argparse.Namespace) -> int:
    missing = [t for t in ("ffmpeg", "ffprobe") if shutil.which(t) is None]
    if missing:
        print(f"error: {' and '.join(missing)} not found on PATH; install ffmpeg", file=sys.stderr)
        return 2

    source = args.source_dir.resolve()
    card = args.card_dir.resolve()
    if not source.is_dir():
        print(f"error: {args.source_dir} is not a directory", file=sys.stderr)
        return 2
    if _is_within(card, source) or _is_within(source, card):
        print("error: SOURCE_DIR and CARD_DIR must not contain one another", file=sys.stderr)
        return 2
    songs = card / "songs"
    if songs.is_symlink():
        print(f"error: {songs} is a symlink; refusing to clear it", file=sys.stderr)
        return 2

    if not args.skip_card_check:
        try:
            check_card(card)
        except CardError as e:
            print(f"error: {e}", file=sys.stderr)
            return 2

    candidates = sorted(
        (p for p in source.iterdir() if p.is_file() and p.suffix.lower() == ".mp3"),
        key=lambda p: p.name.lower(),
    )
    skipped = 0
    sources = []
    print(f"checking {len(candidates)} files...", file=sys.stderr)
    try:
        for p in candidates:
            if audio.is_decodable(p):
                sources.append(p)
            else:
                _warn(f"skipping undecodable file {p.name}")
                skipped += 1
    except audio.AudioError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    if not sources:
        print(f"error: no decodable MP3 files found in {args.source_dir}; card left untouched",
              file=sys.stderr)
        print(f"0 songs written, {skipped} skipped")
        return 1

    try:
        named = assign_names(sources)
    except TooManySongs as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    card.mkdir(parents=True, exist_ok=True)
    # Drop the old index first so an aborted run never leaves one listing missing songs.
    (card / "index.txt").unlink(missing_ok=True)
    _clear_dir(songs)
    entries = []
    total = len(named)
    for i, (src, name) in enumerate(named, 1):
        print(f"[{i}/{total}] {name}", file=sys.stderr)
        try:
            title = audio.probe_title(src) or src.stem
            audio.process(src, songs / name)
        except audio.AudioError as e:
            _warn(f"skipping {src.name}: {e}")
            (songs / name).unlink(missing_ok=True)
            skipped += 1
            continue
        entries.append((name, title))

    write_index(card / "index.txt", entries)
    print(f"{len(entries)} songs written, {skipped} skipped")
    return 0 if entries else 1


if __name__ == "__main__":
    sys.exit(main())
