"""Fixed-width index file read by the firmware by byte offset.

Layout: a HEADER_LEN-byte header, then RECORD_LEN-byte records. Record N
starts at byte offset HEADER_LEN + RECORD_LEN * N.
"""
import os
from pathlib import Path

HEADER_LEN = 32
RECORD_LEN = 128


def encode_header(count: int) -> bytes:
    text = f"SONGBUTTON 1 {count}".encode("ascii")
    return text.ljust(HEADER_LEN - 1) + b"\n"


_CONTROL = str.maketrans({"\t": " ", "\n": " ", "\r": " "})
MAX_FILENAME_LEN = RECORD_LEN - 2  # leaves room for the tab separator


def encode_record(filename: str, title: str) -> bytes:
    if not filename or not filename.isascii():
        raise ValueError(f"filename must be non-empty ASCII: {filename!r}")
    if any(c in filename for c in "\t\n\r"):
        raise ValueError(f"filename contains a control character: {filename!r}")
    if len(filename) > MAX_FILENAME_LEN:
        raise ValueError(f"filename longer than {MAX_FILENAME_LEN} bytes: {filename!r}")
    prefix = filename.encode("ascii") + b"\t"
    room = RECORD_LEN - 1 - len(prefix)
    title = title.translate(_CONTROL)
    # Cut on bytes, then drop any partial trailing multi-byte character.
    body = title.encode("utf-8")[:room].decode("utf-8", errors="ignore").encode("utf-8")
    record = (prefix + body).ljust(RECORD_LEN - 1) + b"\n"
    assert len(record) == RECORD_LEN
    return record


def write_index(path: Path, entries: list[tuple[str, str]]) -> None:
    tmp = path.with_suffix(".tmp")
    with open(tmp, "wb") as f:
        f.write(encode_header(len(entries)))
        for filename, title in entries:
            f.write(encode_record(filename, title))
    os.replace(tmp, path)
