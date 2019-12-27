## syaz0
### Library for data compression using the Yaz0 algorithm

*syaz0* is a native module for Python 3.6+ that provides fast data compression and decompression using Nintendo's [Yaz0](https://zeldamods.org/wiki/Yaz0) algorithm.

## Performance

Decompression performance is on par with existing Yaz0 decoders.

As of late December 2019, syaz0 is able to compress files much faster than existing Yaz0 encoders. Files that are representative of *Breath of the Wild* assets were compressed 20x to 30x faster than with existing public tools for an equivalent or better compression ratio, and 70-80x faster (with a slightly worse ratio) in extreme cases.

At the default compression level, file sizes are typically within 1% of Nintendo's.

For detailed benchmarks, see the results files in the test directory.

## Usage

*syaz0* can be installed with `pip3 install syaz0`. Binary builds are provided for Windows 64 bits (only). (On all other platforms, building from source is required. Skip to the end of the README for more information.)

### `syaz0.decompress(data)`

Decompresses Yaz0-compressed data from a bytes-like object `data`. Returns a bytes object containing the uncompressed data.

### `syaz0.decompress_unsafe(data)`

Decompresses Yaz0-compressed data from a bytes (*not* bytes-like) object `data`. Returns a bytes object containing the uncompressed data.

Unlike syaz0.decompress, this function assumes that the input data is well-formed. In exchange for slightly improved performance, no sanity checks are performed. **Warning**: Do not use on untrusted data.

### `syaz0.compress(data, data_alignment=0, level=7)`

Compresses a bytes-like object `data`. Returns a bytes-like object containing the Yaz0-compressed data.

`data_alignment` is a hint for decoders to allocate buffers with the required data alignment. Defaults to 0, which indicates that no particular alignment is required.

`level` is the compression level (6-9). 6 is fastest and 9 is slowest. Higher compression levels result in better compression. 7 is a good compromise between compression ratio and performance.

## Project information

*syaz0* was written with two goals in mind: to improve performance for Yaz0 compression — which is excruciatingly slow if one also desires decent compression ratios — and to let me learn a bit more about compression.

After doing more research on compression algorithms and finding out
just how similar Yaz0 and LZ77-style compression algorithms are,
I tried to implement some common tricks to help improve the extremely
poor compression performance due to the sliding window search.

But the implementation was still extremely slow compared to gzip. And probably badly implemented.

It turns out that many more tricks were needed for fast compression.

After stumbling upon [zlib-ng](https://github.com/zlib-ng/zlib-ng) and seeing how well-optimised it is
and all the intrinsics I decided it was best not to reinvent the wheel.
Thus *syaz0* uses a copy of zlib-ng for all the heavy lifting (match searching). The following modifications were made:

* The window size was reduced to 4K (2^12) to match Yaz0.
* The compress function and the stream structures were changed to take
  a callback that is invoked every time a distance/length pair or a
  literal is emitted. (I'm not proud, but it works.)
* MAX_MATCH was *not* increased. zlib assumes it is equal to 258 in
  too many places and increasing it actually gives worse compression ratios.

### Building from source

Building *syaz0* from source requires:

* CMake 3.10+
* A compiler that supports C++17
* Everything needed to build [zlib-ng](https://github.com/zlib-ng/zlib-ng)
* pybind11 2.4+ (including CMake config files)
* setuptools

When no binary build is available, pip will automatically build from source during the install process.

To build from source manually, run `python3 setup.py bdist_wheel`.

## License

This software is licensed under the terms of the GNU General Public License, version 2 or later.
