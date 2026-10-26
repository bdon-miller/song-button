import re
import unicodedata
from pathlib import Path

MAX_SONGS = 9999
MAX_NAME_LEN = 40
_SUFFIX = ".mp3"
_PREFIX_LEN = len("0001-")
_SLUG_MAX = MAX_NAME_LEN - _PREFIX_LEN - len(_SUFFIX)


class TooManySongs(Exception):
    """Raised when more than 9999 songs are given (the NNNN prefix is 4 digits)."""


def slugify(name: str, max_len: int) -> str:
    """Lowercase ASCII slug of letters, digits and single hyphens, at most max_len chars."""
    if name.lower().endswith(_SUFFIX):
        name = name[: -len(_SUFFIX)]
    ascii_name = unicodedata.normalize("NFKD", name).encode("ascii", "ignore").decode("ascii")
    slug = re.sub(r"[^a-z0-9]+", "-", ascii_name.lower()).strip("-")
    slug = slug[:max_len].strip("-")
    return slug or "track"


def assign_names(sources: list[Path]) -> list[tuple[Path, str]]:
    """Pair each source with `NNNN-slug.mp3` in order; the NNNN prefix guarantees uniqueness."""
    if len(sources) > MAX_SONGS:
        raise TooManySongs(f"{len(sources)} songs exceeds the limit of {MAX_SONGS}")
    return [
        (src, f"{i:04d}-{slugify(src.name, _SLUG_MAX)}{_SUFFIX}")
        for i, src in enumerate(sources, start=1)
    ]
