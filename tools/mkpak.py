#!/usr/bin/env python3
"""
mkpak.py -- packs the Tyrian 2.1 data files into tyrian.pak, the one asset the door sends.

    python3 tools/mkpak.py data door/tyrian.pak

Format (read by module/src/tyrtrace.c), all little-endian:
    "TYRPAK1\\0", u32 count, count x { char name[24]; u32 offset; u32 size }, then the files one after another.

Only what the game reads goes in: the DOS programs, their overlays and the text documents are left out. The files
themselves are unchanged, which is what the freeware licence asks. Names are sorted, so the same data always makes
the same pack, and so the same hash: players who already have it aren't sent it again.
"""
import os
import struct
import sys

SKIP = {'.exe', '.ovl', '.doc', '.diz', '.ico', '.txt', '.int', '.ini', '.box', '.tfp', '.bin'}

def main():
    src, out = sys.argv[1], sys.argv[2]
    names = sorted(n.lower() for n in os.listdir(src)
                   if os.path.isfile(os.path.join(src, n)) and os.path.splitext(n)[1].lower() not in SKIP)
    blobs = []
    for n in names:
        if len(n.encode()) > 23:
            sys.exit(f'{n}: name too long for the pack')
        with open(os.path.join(src, n), 'rb') as f:
            blobs.append(f.read())
    offset = 12 + 32 * len(names)
    table = b''
    for n, b in zip(names, blobs):
        table += struct.pack('<24sII', n.encode(), offset, len(b))
        offset += len(b)
    with open(out, 'wb') as f:
        f.write(b'TYRPAK1\0' + struct.pack('<I', len(names)) + table + b''.join(blobs))
    print(f'{out}: {len(names)} files, {offset} bytes')

if __name__ == '__main__':
    main()
