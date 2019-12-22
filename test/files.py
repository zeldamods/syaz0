from pathlib import Path
import pytest
import libyaz0

TEST_FILES = []
TEST_FILE_DATA = dict()
TEST_FILE_DATA_UNCOMP = dict()
for path in (Path(__file__).parent / "files").glob("*"):
    with path.open("rb") as f:
        TEST_FILES.append(pytest.param(path.name, id='/'))
        TEST_FILE_DATA[path.name] = f.read()
        TEST_FILE_DATA_UNCOMP[path.name] = libyaz0.decompress(TEST_FILE_DATA[path.name])
