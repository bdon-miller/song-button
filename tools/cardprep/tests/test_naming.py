from pathlib import Path

import pytest

from cardprep.naming import TooManySongs, assign_names, slugify


def test_slugify_ascii_and_length():
    assert slugify("Héllo  World!.mp3", 10) == "hello-worl"


def test_slugify_empty_falls_back():
    assert slugify("日本語.mp3", 10) == "track"


def test_slugify_strips_extension_case_insensitively():
    assert slugify("Two.MP3", 20) == "two"


def test_assign_names_unique_and_bounded():
    # Same slug for both sources; uniqueness comes from the NNNN- prefix.
    pairs = assign_names([Path("Song.mp3"), Path("song.MP3")])
    names = [n for _, n in pairs]
    assert names == ["0001-song.mp3", "0002-song.mp3"]
    assert all(len(n) <= 40 and n.isascii() for n in names)


def test_assign_names_truncates_long_names_to_40_chars():
    pairs = assign_names([Path("a" * 100 + ".mp3")])
    assert len(pairs[0][1]) == 40
    assert pairs[0][1].startswith("0001-aaaa")
    assert pairs[0][1].endswith(".mp3")


def test_assign_names_rejects_over_9999():
    with pytest.raises(TooManySongs):
        assign_names([Path(f"{i}.mp3") for i in range(10000)])
