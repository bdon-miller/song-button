# cardprep

Prepares a microSD card of MP3s for the one-button random song player.

## Install

Requires Python 3.12+ and ffmpeg (which provides ffprobe). Supports macOS and Linux only.

```
brew install ffmpeg
python3 -m venv .venv && .venv/bin/pip install -e .
```

## Usage

```
.venv/bin/cardprep SOURCE_DIR CARD_DIR [--skip-card-check]
```

On macOS the card path is `/Volumes/<CARDNAME>`, for example `.venv/bin/cardprep ~/Music/songs /Volumes/SONGCARD`.

- Collects the `*.mp3` files in SOURCE_DIR (top level, sorted by name) and skips any that will not decode.
- Trims leading silence and normalises loudness, writing `CARD_DIR/songs/NNNN-name.mp3`.
- Replaces the contents of `CARD_DIR/songs/` on every run and never touches SOURCE_DIR.
- Writes `CARD_DIR/index.txt` last, listing each song's filename and title.

Exit codes: 0 success, 1 no songs written, 2 refused or error.

The card must be FAT32 and 32 GB or smaller; `--skip-card-check` bypasses this check.

If you drag files onto an already-prepared card, delete index.txt from the card; the player rebuilds it on the next press (that press will be slow).
