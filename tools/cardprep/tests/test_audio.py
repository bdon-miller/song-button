from cardprep.audio import duration, is_decodable, measure_lufs, probe_title, process


def test_probe_title(tone_mp3):
    assert probe_title(tone_mp3(title="Hello")) == "Hello"


def test_corrupt_file_not_decodable(tmp_path):
    bad = tmp_path / "bad.mp3"
    bad.write_bytes(b"not an mp3")
    assert is_decodable(bad) is False


def test_real_mp3_decodable(tone_mp3):
    assert is_decodable(tone_mp3()) is True


def test_process_strips_tags_and_trims_silence(tone_mp3, tmp_path):
    src = tone_mp3(seconds=2.0, lead_silence=1.0, title="T")
    dst = tmp_path / "out.mp3"
    process(src, dst)
    assert probe_title(dst) is None
    assert 1.8 < duration(dst) < 2.3  # the 1 s of silence is gone


def test_process_normalises_loudness(tone_mp3, tmp_path):
    dst = tmp_path / "out.mp3"
    process(tone_mp3(seconds=3.0), dst)
    assert -18.0 < measure_lufs(dst) < -14.0
