#!/usr/bin/env python3

import argparse
import functools
import struct

parser = argparse.ArgumentParser(description='TimeMachine IPL Payload Creator')

parser.add_argument('--input_payload', help='Payload binary', type=str, required=True)
parser.add_argument('--payload_addr', help='Destination address of the payload', type=functools.partial(int, base=16), required=True)
parser.add_argument('--ipl_addr', help='Destination address of the Sony IPL', type=functools.partial(int, base=16), default=0)
parser.add_argument('--payload_is_entry', help='Start execution from payload instead of Sony IPL.', action='store_true', default=False)
parser.add_argument('--output', help='Output file', type=str, required=True)

args = parser.parse_args()

def payload_header(payload_addr, ipl_addr, payload_is_entry):
    return struct.pack('<III', payload_addr, ipl_addr, 1 if payload_is_entry else 0)

with open(args.input_payload, 'rb') as f:
    payload_code = f.read()

header = payload_header(args.payload_addr, args.ipl_addr, args.payload_is_entry)

with open(args.output, 'wb') as f:
    f.write(header + payload_code)
