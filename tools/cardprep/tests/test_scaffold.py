def test_fixture_makes_decodable_mp3(tone_mp3):
    p = tone_mp3(seconds=1.0)
    assert p.suffix == ".mp3" and p.stat().st_size > 1000
