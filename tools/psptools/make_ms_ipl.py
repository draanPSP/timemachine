#!/usr/bin/env python3

import argparse
import struct
import heatshrink2
from psptool.kirk import kirk1_encrypt_cmac, kirk4
from Crypto.Hash import SHA1

parser = argparse.ArgumentParser(description='TimeMachine MS IPL Creator')

parser.add_argument('--legacy', help='Create legacy-style IPL which allows bootrom access, but works only up to TA-088v2', action='store_true')
parser.add_argument('--input_loader', help='Loader binary', type=str, required=True)
parser.add_argument('--input_payload', help='Payload binary', type=str, required=True)
parser.add_argument('--window_sz2', help='Heatshrink window size', type=int, required=True)
parser.add_argument('--lookahead_sz2', help='Heatshrink lookahead size', type=int, required=True)
parser.add_argument('--output', help='Output file', type=str, required=True)

args = parser.parse_args()

def ipl_header(block_destination, size, entry, hash):
    return struct.pack('<IIII', block_destination, size, entry, hash)

with open(args.input_loader, 'rb') as f:
    loader_code = f.read()

with open(args.input_payload, 'rb') as f:
    payload_code = f.read()

payload_code_compressed = heatshrink2.compress(payload_code, window_sz2=args.window_sz2, lookahead_sz2=args.lookahead_sz2)

if args.legacy:
    header = ipl_header(0xBFD00010, 4, 0xBFD00014, 0) + struct.pack('<I', 0) #header + block payload (a single nop instruction)
    block = header + loader_code
    block_enc = kirk1_encrypt_cmac(block)

    padding = bytearray(0x1000 - len(block_enc)) #pad with zeros to IPL block size, i.e. 0x1000 bytes
    block_enc = block_enc + padding
else:
    header = ipl_header(0xBC10004C, 4, 0x10000004, 0) + struct.pack('<III', 0x32F6, 0, 0) #reset exploit; 0x32F6 is USB_HOST, ATA_HDD, MS_1, ATA, USB, AVC, VME, SC, ME reset bit
    block = header + loader_code
    block_enc = kirk1_encrypt_cmac(block)

    sha_buf = block[8:8+12] + block[:8]

    block_hash = SHA1.new(sha_buf).digest()
    hash_padding = bytearray(12)
    hash_enc = kirk4(block_hash + hash_padding, 0x6C)

    padding = bytearray(0x1000 - len(block_enc) - 0x20) #pad with zeros to IPL block size minus the SHA1 hash at the end
    block_enc = block_enc + padding + hash_enc

with open(args.output, 'wb') as f:
    f.write(block_enc + payload_code_compressed)
