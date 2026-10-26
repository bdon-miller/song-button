import shutil

import pytest

from cardprep import audio
from cardprep.cli import main


def _src(tmp_path, name="src"):
    src = tmp_path / name
    src.mkdir()
    return src


def test_cli_end_to_end(tone_mp3, tmp_path):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(title="One"), src / "One.mp3")
    shutil.copy(tone_mp3(), src / "Two.MP3")
    (src / "bad.mp3").write_bytes(b"nope")
    card = tmp_path / "card"
    card.mkdir()
    assert main([str(src), str(card), "--skip-card-check"]) == 0
    assert sorted(p.name for p in (card / "songs").iterdir()) == ["0001-one.mp3", "0002-two.mp3"]
    data = (card / "index.txt").read_bytes()
    assert data.startswith(b"SONGBUTTON 1 2")
    assert b"One" in data and b"Two" in data
    assert not (card / "index.tmp").exists()


def test_cli_reports_counts(tone_mp3, tmp_path, capsys):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    (src / "bad.mp3").write_bytes(b"nope")
    card = tmp_path / "card"
    assert main([str(src), str(card), "--skip-card-check"]) == 0
    captured = capsys.readouterr()
    assert "1 songs written, 1 skipped" in captured.out
    assert "bad.mp3" in captured.err


def test_cli_source_untouched(tone_mp3, tmp_path):
    src = _src(tmp_path)
    f = src / "A.mp3"
    shutil.copy(tone_mp3(), f)
    before = f.read_bytes()
    main([str(src), str(tmp_path / "card"), "--skip-card-check"])
    assert f.read_bytes() == before
    assert [p.name for p in src.iterdir()] == ["A.mp3"]


def test_cli_rerun_replaces_cleanly(tone_mp3, tmp_path):
    card = tmp_path / "card"
    card.mkdir()
    (card / "keep.txt").write_text("not ours")
    src1 = _src(tmp_path, "src1")
    shutil.copy(tone_mp3(), src1 / "Alpha.mp3")
    shutil.copy(tone_mp3(), src1 / "Beta.mp3")
    assert main([str(src1), str(card), "--skip-card-check"]) == 0
    assert (card / "index.txt").read_bytes().startswith(b"SONGBUTTON 1 2")

    src2 = _src(tmp_path, "src2")
    shutil.copy(tone_mp3(title="Gamma Song"), src2 / "Gamma.mp3")
    assert main([str(src2), str(card), "--skip-card-check"]) == 0
    assert sorted(p.name for p in (card / "songs").iterdir()) == ["0001-gamma.mp3"]
    data = (card / "index.txt").read_bytes()
    assert data.startswith(b"SONGBUTTON 1 1 ")
    assert b"Gamma Song" in data and b"Alpha" not in data
    assert len(data) == 32 + 128
    assert (card / "keep.txt").read_text() == "not ours"


def test_cli_zero_songs_exits_1(tmp_path):
    src = _src(tmp_path)
    (src / "bad.mp3").write_bytes(b"nope")
    assert main([str(src), str(tmp_path / "card"), "--skip-card-check"]) == 1


def test_cli_refuses_nested_dirs(tone_mp3, tmp_path):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    assert main([str(src), str(src / "card"), "--skip-card-check"]) == 2
    assert not (src / "card").exists()
    assert main([str(src), str(tmp_path), "--skip-card-check"]) == 2
    assert main([str(src), str(src), "--skip-card-check"]) == 2


def test_cli_refuses_case_variant_nested_dirs(tone_mp3, tmp_path):
    src = tmp_path / "Music"
    src.mkdir()
    shutil.copy(tone_mp3(), src / "A.mp3")
    variant = tmp_path / "music"
    if not variant.exists():
        pytest.skip("filesystem is case-sensitive")
    assert main([str(src), str(variant), "--skip-card-check"]) == 2
    assert not (src / "songs").exists()
    assert not (src / "index.txt").exists()
    assert [p.name for p in src.iterdir()] == ["A.mp3"]


def test_cli_card_check_failure_exits_2(tone_mp3, tmp_path):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    card = tmp_path / "card"
    card.mkdir()
    assert main([str(src), str(card)]) == 2
    assert not (card / "songs").exists()


def _prepared_card(tone_mp3, tmp_path):
    src = _src(tmp_path, "src0")
    shutil.copy(tone_mp3(title="Old"), src / "Old.mp3")
    card = tmp_path / "card"
    assert main([str(src), str(card), "--skip-card-check"]) == 0
    return card


def test_rerun_abort_removes_stale_index(tone_mp3, tmp_path, monkeypatch):
    card = _prepared_card(tone_mp3, tmp_path)
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    shutil.copy(tone_mp3(), src / "B.mp3")

    def boom(*a, **k):
        raise RuntimeError("boom")

    monkeypatch.setattr("cardprep.cli.audio.process", boom)
    assert main([str(src), str(card), "--skip-card-check"]) == 2
    assert not (card / "index.txt").exists()


def test_zero_decodable_songs_leaves_card_untouched(tone_mp3, tmp_path, capsys):
    card = _prepared_card(tone_mp3, tmp_path)
    index_before = (card / "index.txt").read_bytes()
    src = _src(tmp_path)
    (src / "bad.mp3").write_bytes(b"nope")
    assert main([str(src), str(card), "--skip-card-check"]) == 1
    assert [p.name for p in (card / "songs").iterdir()] == ["0001-old.mp3"]
    assert (card / "index.txt").read_bytes() == index_before
    assert capsys.readouterr().err


def test_missing_ffmpeg_exits_2(tone_mp3, tmp_path, monkeypatch, capsys):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    monkeypatch.setattr(shutil, "which", lambda name: None)
    card = tmp_path / "card"
    assert main([str(src), str(card), "--skip-card-check"]) == 2
    assert "ffmpeg" in capsys.readouterr().err
    assert not card.exists()


def test_symlinked_songs_dir_refused(tone_mp3, tmp_path):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    victim = tmp_path / "victim"
    victim.mkdir()
    (victim / "precious.txt").write_text("keep")
    card = tmp_path / "card"
    card.mkdir()
    (card / "songs").symlink_to(victim, target_is_directory=True)
    assert main([str(src), str(card), "--skip-card-check"]) == 2
    assert (victim / "precious.txt").read_text() == "keep"


def test_missing_card_path_with_check_on_exits_2(tone_mp3, tmp_path):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    card = tmp_path / "nocard"
    assert main([str(src), str(card)]) == 2
    assert not card.exists()


def test_audio_error_skips_file_and_index_lists_only_written(tone_mp3, tmp_path, monkeypatch, capsys):
    src = _src(tmp_path)
    shutil.copy(tone_mp3(), src / "A.mp3")
    shutil.copy(tone_mp3(), src / "B.mp3")
    real = audio.process

    def flaky(s, d):
        if s.name == "A.mp3":
            d.write_bytes(b"partial")
            raise audio.AudioError("nope")
        real(s, d)

    monkeypatch.setattr("cardprep.cli.audio.process", flaky)
    card = tmp_path / "card"
    assert main([str(src), str(card), "--skip-card-check"]) == 0
    assert "1 songs written, 1 skipped" in capsys.readouterr().out
    assert (card / "index.txt").read_bytes().startswith(b"SONGBUTTON 1 1 ")
    assert [p.name for p in (card / "songs").iterdir()] == ["0002-b.mp3"]
