#!/usr/bin/env python3
import argparse
from pathlib import Path
import syaz0
import sys


def main() -> None:
    parser = argparse.ArgumentParser("syaz0", description="Compress or decompress Yaz0 data")
    parser.add_argument("-d", "--decompress", help="Decompress", action="store_true", default=False)
    parser.add_argument("-c", "--stdout", help="Write output to stdout", action="store_true", default=False)
    parser.add_argument("-l", "--level", help="Compression level (6-9)",
                        type=int, choices=[6, 7, 8, 9], default=9)
    parser.add_argument("-a", "--alignment",
                        help="Data alignment hint (for compression)", type=int, default=0)
    parser.add_argument("file")

    args = parser.parse_args()

    with open(args.file, "rb") as f:
        data = f.read()

    if args.decompress or args.file.endswith(".yaz0"):
        result = syaz0.decompress(data)
        name = args.file.replace(".yaz0", "")
        if Path(name).exists():
            name += ".decomp"
    else:
        result = syaz0.compress(data, data_alignment=args.alignment, level=args.level)
        name = args.file + ".yaz0"

    if args.stdout:
        sys.stdout.buffer.write(result)
    else:
        with open(name, "wb") as f:
            f.write(result)

main()
