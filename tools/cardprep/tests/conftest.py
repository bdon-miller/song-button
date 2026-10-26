import itertools
import shutil
import subprocess
from pathlib import Path

import pytest


def pytest_collection_modifyitems(config, items):
    if shutil.which("ffmpeg") is None:
        skip = pytest.mark.skip(reason="ffmpeg not found on PATH; install ffmpeg to run the cardprep suite")
        for item in items:
            item.add_marker(skip)


@pytest.fixture
def tone_mp3(tmp_path_factory):
    """Factory for short synthetic MP3s (440 Hz sine), optionally preceded by digital silence.

    With lead_silence > 0 the file is `lead_silence` seconds longer than `seconds`.
    """
    out_dir = tmp_path_factory.mktemp("tones")
    counter = itertools.count(1)

    def make(seconds: float = 2.0, lead_silence: float = 0.0, title: str | None = None) -> Path:
        path = out_dir / f"tone{next(counter)}.mp3"
        cmd = ["ffmpeg", "-nostdin", "-loglevel", "error", "-y",
               "-f", "lavfi", "-i", f"sine=frequency=440:duration={seconds}"]
        if lead_silence > 0:
            cmd += ["-af", f"adelay={int(round(lead_silence * 1000))}:all=1"]
        if title is not None:
            cmd += ["-metadata", f"title={title}"]
        cmd += ["-c:a", "libmp3lame", str(path)]
        subprocess.run(cmd, check=True, capture_output=True)
        return path

    return make
