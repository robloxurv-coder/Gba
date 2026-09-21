#!/usr/bin/env python3
"""Finalize/validate the mandatory GBA cartridge header; no external SDK needed."""
import argparse
from pathlib import Path

LOGO = bytes.fromhex('''
24 ff ae 51 69 9a a2 21 3d 84 82 0a 84 e4 09 ad
11 24 8b 98 c0 81 7f 21 a3 52 be 19 93 09 ce 20
10 46 4a 4a f8 27 31 ec 58 c7 e8 33 82 e3 ce bf
85 f4 df 94 ce 4b 09 c1 94 56 8a c0 13 72 a7 fc
9f 84 4d 73 a3 ca 9a 61 58 97 a3 27 fc 03 98 76
23 1d c7 61 03 04 ae 56 bf 38 84 00 40 a7 0e fd
ff 52 fe 03 6f 95 30 f1 97 fb c0 85 60 d6 80 25
a9 63 be 03 01 4e 38 e2 f9 a2 34 ff bb 3e 03 44
78 00 90 cb 88 11 3a 94 65 c0 7c 63 87 f0 3c af
d6 25 e4 8b 38 0a ac 72 21 d4 f8 07''')

def validate(data):
    assert 192 <= len(data) <= 32 * 1024 * 1024, 'Invalid ROM size'
    assert len(data) & (len(data)-1) == 0, 'ROM must be padded to a power of two'
    assert data[3] == 0xEA, 'Missing ARM entry branch'
    assert data[4:0xA0] == LOGO, 'Invalid boot logo'
    assert data[0xB2] == 0x96, 'Invalid fixed byte'
    assert (sum(data[0xA0:0xBE]) + 0x19) & 255 == 0, 'Invalid header checksum'
    print(f'ROM OK: {len(data):,} bytes, title={data[0xA0:0xAC].decode().strip()}')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    data = bytearray(args.rom.read_bytes())
    if not args.check:
        if len(data)<192:
            raise ValueError('ROM is missing its reserved header')
        data[4:0xA0]=LOGO
        data[0xA0:0xAC]=b'ILHA ABRIGO '
        data[0xAC:0xB0]=b'IABR'
        data[0xB0:0xB2]=b'00'
        data[0xB2]=0x96
        data[0xB3:0xBD]=bytes(10)
        data[0xBD]=(-sum(data[0xA0:0xBD])-0x19)&255
        data[0xBE:0xC0]=b'\0\0'
        size=max(32768,1<<(len(data)-1).bit_length())
        data.extend(b'\xff'*(size-len(data)))
        args.rom.write_bytes(data)
    validate(data)

if __name__=='__main__':
    main()
