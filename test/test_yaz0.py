import pytest
import syaz0
import wszst_yaz0
from files import TEST_FILES, TEST_FILE_DATA, TEST_FILE_DATA_UNCOMP


@pytest.mark.parametrize("file", TEST_FILES)
def test_decompression(file):
    assert TEST_FILE_DATA_UNCOMP[file] == syaz0.decompress(TEST_FILE_DATA[file])


@pytest.mark.parametrize("file", TEST_FILES)
def test_roundtrip(file):
    assert TEST_FILE_DATA_UNCOMP[file] == syaz0.decompress(syaz0.compress(TEST_FILE_DATA_UNCOMP[file]))

