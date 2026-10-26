"""ffmpeg/ffprobe wrappers for preparing song files."""
import json
import subprocess
from pathlib import Path

_LOUDNORM = "loudnorm=I=-16:TP=-1.5:LRA=11"
_FILTER = f"silenceremove=start_periods=1:start_threshold=-50dB,{_LOUDNORM}"


class AudioError(Exception):
    pass


def _run(cmd: list[str]) -> subprocess.CompletedProcess:
    try:
        return subprocess.run(cmd, capture_output=True, text=True, stdin=subprocess.DEVNULL)
    except FileNotFoundError as e:
        raise AudioError(f"{cmd[0]} not found; install ffmpeg") from e


def _probe_format(path: Path) -> dict:
    r = _run(["ffprobe", "-v", "error", "-show_entries", "format=duration:format_tags=title",
              "-of", "json", str(path)])
    if r.returncode != 0:
        raise AudioError(r.stderr.strip())
    return json.loads(r.stdout).get("format", {})


def probe_title(src: Path) -> str | None:
    tags = _probe_format(src).get("tags", {})
    title = next((v for k, v in tags.items() if k.lower() == "title"), None)
    return title or None


def duration(path: Path) -> float:
    return float(_probe_format(path)["duration"])


def is_decodable(src: Path) -> bool:
    r = _run(["ffmpeg", "-nostdin", "-v", "error", "-i", str(src), "-vn", "-f", "null", "-"])
    return r.returncode == 0


def process(src: Path, dst: Path) -> None:
    r = _run(["ffmpeg", "-nostdin", "-y", "-hide_banner", "-loglevel", "error", "-i", str(src),
              "-vn", "-map_metadata", "-1", "-af", _FILTER,
              "-ar", "44100", "-c:a", "libmp3lame", "-b:a", "192k", str(dst)])
    if r.returncode != 0:
        raise AudioError(f"ffmpeg failed for {src}: {r.stderr.strip()}")


def measure_lufs(path: Path) -> float:
    r = _run(["ffmpeg", "-nostdin", "-hide_banner", "-i", str(path),
              "-af", f"{_LOUDNORM}:print_format=json", "-f", "null", "-"])
    if r.returncode != 0:
        raise AudioError(r.stderr.strip())
    start = r.stderr.rfind("{")
    end = r.stderr.rfind("}")
    if start == -1 or end < start:
        raise AudioError(f"no loudnorm JSON in ffmpeg output: {r.stderr.strip()}")
    return float(json.loads(r.stderr[start:end + 1])["input_i"])
