import pytest

from cardprep.card import CardError, check_card


def test_check_card_rejects_non_fat32(tmp_path):
    with pytest.raises(CardError):
        check_card(tmp_path)  # tmp is never FAT32


def test_check_card_missing_path_is_card_error(tmp_path):
    with pytest.raises(CardError):
        check_card(tmp_path / "does-not-exist")
