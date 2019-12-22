import pytest
import libyaz0
import syaz0
import wszst_yaz0
from files import TEST_FILES, TEST_FILE_DATA, TEST_FILE_DATA_UNCOMP


@pytest.mark.parametrize("file", TEST_FILES)
def test_comp_syaz0(benchmark, file):
    benchmark.group = "comp: " + file
    benchmark(syaz0.compress, TEST_FILE_DATA_UNCOMP[file])


@pytest.mark.parametrize("file", TEST_FILES)
def test_comp_wszst_yaz0(benchmark, file):
    benchmark.group = "comp: " + file
    benchmark(wszst_yaz0.compress, TEST_FILE_DATA_UNCOMP[file])


@pytest.mark.parametrize("file", TEST_FILES)
def test_comp_libyaz0(benchmark, file):
    benchmark.group = "comp: " + file
    benchmark(libyaz0.compress, TEST_FILE_DATA_UNCOMP[file], level=10)
