#!/usr/bin/env python3
"""
Prints functions of Ghidra's decompilation with the literal pool resolved.

  show.py <address> [<address> ...]

Each DAT_xxxxxxxx / PTR_..._xxxxxxxx the text names is read from arm9_main.bin and listed under
the function: its 32-bit value, and the string it points to if it is one.
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
RE = os.path.dirname(os.path.dirname(HERE))
BASE = 0x02000000
BIN = open(os.path.join(RE, 'load', 'arm9_main.bin'), 'rb').read()
HEAD = re.compile(r'^// ([0-9a-f]{8}) (\S+)\s*$')
REF = re.compile(r'\b(?:DAT|PTR_\w*?)_([0-9a-f]{8})\b')


def u32(a):
    o = a - BASE
    return struct.unpack_from('<I', BIN, o)[0] if 0 <= o <= len(BIN) - 4 else None


def cstr(a):
    o = a - BASE
    if not 0 <= o < len(BIN):
        return None
    e = BIN.find(b'\0', o)
    s = BIN[o:e]
    if len(s) >= 2 and all(32 <= c < 127 or c in (9, 10) for c in s):
        return s.decode('latin1')
    return None


def funcs():
    out, cur = {}, None
    for line in open(os.path.join(RE, 'arm9_usa.c'), encoding='utf-8', errors='replace'):
        m = HEAD.match(line)
        if m:
            cur = int(m.group(1), 16)
            out[cur] = []
        elif cur is not None:
            out[cur].append(line.rstrip('\n'))
    return out


def main():
    fs = funcs()
    for arg in sys.argv[1:]:
        a = int(arg, 16)
        text = '\n'.join(fs.get(a, ['(no such function)'])).strip('\n')
        print('==== %08x' % a)
        print(text)
        seen = []
        for m in REF.finditer(text):
            r = int(m.group(1), 16)
            if r not in seen:
                seen.append(r)
        for r in seen:
            v = u32(r)
            if v is None:
                continue
            s = cstr(v)
            print('  [%08x] = %08x%s' % (r, v, '  "%s"' % s.replace('\n', '\\n') if s else ''))
        print()


if __name__ == '__main__':
    main()
