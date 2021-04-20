#!/usr/bin/env python3

import argparse
import hashlib
import gzip

def readfile(file):
    if file[0] == '*':
        return bytearray.fromhex(file[1:])
    else:
        with open(file, 'rb') as f:
            return f.read()

def split_values(x):
    s = x.split('=')
    return (s[0], readfile(s[1]))

def generate_hpp(name, data):
    return f'namespace {name} {{\nextern std::array<unsigned char, {len(data)}> data;\n}}\n'

def generate_only_hpp(name, data, align):
    c_bytes = ", ".join(['0x{:02X}'.format(x) for x in data])
    alignas = f'alignas({align}) ' if align is not None else ''
    return f'constexpr {alignas}std::array<unsigned char, {len(data)}> data = {{ {c_bytes} }};\n'

def generate_cpp(name, data, align):
    c_bytes = ", ".join(['0x{:02X}'.format(x) for x in data])
    alignas = f'alignas({align}) ' if align is not None else ''
    return f'namespace {name} {{\n{alignas}std::array<unsigned char, {len(data)}> data = {{ {c_bytes} }};\n}}\n'

def generate_h(name, data):
    return f'extern unsigned char {name}[{len(data)}];\n'

def generate_only_h(name, data, align):
    c_bytes = ", ".join(['0x{:02X}'.format(x) for x in data])
    alignas = f'alignas({align}) ' if align is not None else ''
    return f'const {alignas}unsigned char {name}[{len(data)}] = {{ {c_bytes} }};\n'

def generate_c(name, data, align):
    c_bytes = ", ".join(['0x{:02X}'.format(x) for x in data])
    alignas = f'alignas({align}) ' if align is not None else ''
    return f'{alignas}unsigned char {name}[{len(data)}] = {{ {c_bytes} }};\n'

parser = argparse.ArgumentParser(description="bin2cpp, embed files in your binaries")
parser.add_argument('files', metavar='NAME=FILE', type=split_values, nargs='+',
                    help='Key value pair of names to files to generate arrays for')
parser.add_argument('--alignas', type=int, required=False, help='Emit the alignment attribute')
parser.add_argument('--c_style', required=False, action='store_true', default=False, help='Emit c style arrays')
parser.add_argument('--gzip', required=False, action='store_true', default=False, help='Gzip the file before embedding')
parser.add_argument('header_output', type=argparse.FileType('w'), help='Location to store the output header file')
parser.add_argument('--source_output', type=argparse.FileType('w'), required=False, help='Location to store the output source file')
args = parser.parse_args()

args.header_output.write('#pragma once\n\n')

if not args.c_style:
    args.header_output.write('#include <array>\n\n')
    args.header_output.write('namespace files {\n')

    if args.source_output is not None:
        args.source_output.write('#include <array>\n\n')
        args.source_output.write('namespace files {\n')

for entry in args.files:
    name, data = entry

    if args.gzip:
        data = gzip.compress(data)

    if args.c_style:
        if args.source_output is not None:
            args.header_output.write(generate_h(name, data))
            args.source_output.write(generate_c(name, data, args.alignas))
        else:
            args.header_output.write(generate_only_h(name, data, args.alignas))
    else:
        if args.source_output is not None:
            args.header_output.write(generate_hpp(name, data))
            args.source_output.write(generate_cpp(name, data, args.alignas))
        else:
            args.header_output.write(generate_only_hpp(name, data, args.alignas))


if not args.c_style:
    args.header_output.write('}\n')
    args.source_output.write('}\n')
