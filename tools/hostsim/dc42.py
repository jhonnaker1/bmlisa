#!/usr/bin/env python3
"""dc42.py - LisaEm's hard disk images (Disk Copy 4.2) for BMLisa's tests.

  dc42.py RAW DC42       makes DC42 from a raw ProFile image (IDLE's: blocks
                         of 532 bytes, 20 of tags then 512 of data)
  dc42.py --check DC42   exit status 0 if its checksums are right

LisaEm's layout (its src/storage/profile.c and libdc42): an 84 byte header
(a name, then the data's and the tags' sizes and checksums, big endian),
all the blocks' data, then all their tags; in each 16, ProFile block b is at
(b * 13) mod 16 (deinterleave5()). The checksum adds each big endian word
and rotates the sum right a bit; the tags' leaves out the first block's.
"""

import struct
import sys


def index(block):
    return (block & ~15) | ((block * 13) & 15)


def checksum(data):
    s = 0
    for i in range(0, len(data) - 1, 2):
        s = (s + ((data[i] << 8) | data[i + 1])) & 0xffffffff
        s = ((s >> 1) | (s << 31)) & 0xffffffff
    return s


def make(raw_path, dc42_path):
    raw = open(raw_path, "rb").read()
    n = len(raw) // 532
    data = bytearray(n * 512)
    tags = bytearray(n * 20)
    for b in range(n):
        k = index(b)
        tags[k * 20:k * 20 + 20] = raw[b * 532:b * 532 + 20]
        data[k * 512:k * 512 + 512] = raw[b * 532 + 20:b * 532 + 532]
    name = b"-lisaem.sunder.net hd-"
    head = bytearray(84)
    head[0] = len(name)
    head[1:1 + len(name)] = name
    head[64:80] = struct.pack(">IIII", n * 512, n * 20, checksum(data), checksum(tags[20:]))
    head[80:84] = bytes((0x5d, 0x93, 0x01, 0x00))
    open(dc42_path, "wb").write(bytes(head) + bytes(data) + bytes(tags))


def check(dc42_path):
    d = open(dc42_path, "rb").read()
    data_size, tag_size, data_sum, tag_sum = struct.unpack(">IIII", d[64:80])
    data = d[84:84 + data_size]
    tags = d[84 + data_size:84 + data_size + tag_size]
    return checksum(data) == data_sum and checksum(tags[20:]) == tag_sum


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--check":
        sys.exit(0 if check(sys.argv[2]) else 1)
    if len(sys.argv) == 3:
        make(sys.argv[1], sys.argv[2])
        sys.exit(0)
    sys.exit(__doc__)
