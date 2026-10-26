import pytest
from cardprep.index import (
    HEADER_LEN,
    RECORD_LEN,
    encode_header,
    encode_record,
    write_index,
)


def test_constants():
    assert HEADER_LEN == 32 and RECORD_LEN == 128


def test_header_is_32_bytes_and_parsable():
    h = encode_header(1000)
    assert len(h) == 32 and h.startswith(b"SONGBUTTON 1 1000") and h.endswith(b"\n")


def test_record_is_128_bytes():
    r = encode_record("0001-song.mp3", "Song")
    assert len(r) == 128 and r == b"0001-song.mp3\tSong".ljust(127) + b"\n"


def test_record_truncates_on_utf8_boundary():
    r = encode_record("0001-a.mp3", "é" * 200)
    assert len(r) == 128
    r.rstrip(b" \n").split(b"\t")[1].decode("utf-8")  # must not raise


def test_write_index_offsets(tmp_path):
    p = tmp_path / "index.txt"
    write_index(p, [("0001-a.mp3", "A"), ("0002-b.mp3", "B")])
    data = p.read_bytes()
    assert data[32:32 + 128].startswith(b"0001-a.mp3\tA")
    assert data[32 + 128:32 + 256].startswith(b"0002-b.mp3\tB")
    assert not p.with_suffix(".tmp").exists()


def test_record_truncation_splits_multibyte_char_with_odd_budget():
    r = encode_record("a" * 11, "日" * 100)  # prefix 12, room 115 (odd, splits a char)
    prefix = b"a" * 11 + b"\t"
    assert 127 - len(prefix) == 115
    assert len(r) == 128
    expected = prefix + "日".encode() * 38  # 114 bytes; 115th would be partial
    assert r == expected.ljust(127) + b"\n"
    r[len(prefix):].rstrip(b" \n").decode("utf-8")


def test_header_count_zero():
    h = encode_header(0)
    assert len(h) == 32 and h.startswith(b"SONGBUTTON 1 0 ") and h.endswith(b"\n")


def test_max_length_filename_leaves_no_room_for_title():
    r = encode_record("a" * 126, "title")
    assert len(r) == 128 and r == b"a" * 126 + b"\t" + b"\n"


@pytest.mark.parametrize("name", ["a" * 127, "", "é.mp3", "a\tb.mp3", "a\nb.mp3", "a\rb.mp3"])
def test_bad_filename_raises(name):
    with pytest.raises(ValueError):
        encode_record(name, "t")


def test_title_control_chars_become_spaces():
    r = encode_record("a.mp3", "x\ty\nz\rw")
    assert len(r) == 128 and r == b"a.mp3\tx y z w".ljust(127) + b"\n"
    assert r.count(b"\n") == 1 and r.count(b"\t") == 1


def test_write_index_empty(tmp_path):
    p = tmp_path / "index.txt"
    write_index(p, [])
    assert p.read_bytes() == encode_header(0)
