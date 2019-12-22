import libyaz0
import syaz0
import wszst_yaz0
from files import TEST_FILES, TEST_FILE_DATA, TEST_FILE_DATA_UNCOMP
import time

RESET = "\33[1;0m"
WHITE = "\33[1;37m"
RED = "\33[1;91m"
GREEN = "\33[1;92m"

for name in TEST_FILE_DATA.keys():
    nintendo_len = len(TEST_FILE_DATA[name])
    syaz0_len = len(syaz0.compress(TEST_FILE_DATA_UNCOMP[name]))
    diff = syaz0_len - nintendo_len
    ratio = syaz0_len / nintendo_len
    percentage = (ratio - 1) * 100.0
    color = GREEN if ratio <= 1.0 else RED
    print(f"{WHITE}{name}{RESET}: Nintendo {nintendo_len}, syaz0 {syaz0_len}, diff {color}{diff}{RESET} ({color}{percentage:.2f}%{RESET})")
