#!/usr/bin/env python3
"""
Builds the oeds-re tree from Ghidra's output for the Orcs & Elves DS arm9 binary.

  build_tree.py [--decomp ../arm9_usa.c] [--out ..]

The arm9 binary keeps each source file's functions together, in link order
(about the order of the .cpp names: App, Applet, Camera, Canvas, Combat, ...).
Ghidra's names (from strings, assert texts and the matches with the BREW
version, oe-re) give a class for a third of the functions; the rest are put
into the file of a neighbour: between two named functions of one class they
are that class's, and between two classes A and B the cut is placed where the
calls (callers and callees of each function) agree best.

Output:
  map/functions.tsv   address, size, file, name, BREW counterpart, how the file was found
  map/files.tsv       the files: address range, functions, restored
  src/<File>.c        every function of the file in address order: a header comment, then the
                      restored C (from restored/<address>.c), or the raw decompilation in #if 0
  src/sdk/            the Nitro SDK and C library before and after the game code (raw)

A function is restored by writing restored/<address>.c (its C, with its comment);
build_tree.py then puts it in place of the raw text.
"""
import argparse, collections, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# class prefixes of names -> source file (the .cpp names the binary has, the others after BREW's)
FILE_OF_CLASS = {
    'App': 'App', 'Applet': 'Applet', 'Camera': 'Camera', 'Canvas': 'Canvas', 'Combat': 'Combat',
    'CombatEntity': 'CombatEntity', 'Weapon': 'CombatEntity', 'Entity': 'Entity',
    'EntityDefManager': 'EntityDefManager', 'EntityDefs': 'EntityDefManager',
    'EntityMonster': 'EntityMonster', 'MonsterGoal': 'EntityMonster', 'Game': 'Game',
    'LerpSprite': 'Game', 'Hud': 'Hud', 'Player': 'Player', 'Render': 'Render',
    'ScriptThread': 'ScriptThread', 'Random': 'Random', 'Stream': 'Stream', 'OutputStream': 'Stream',
    'OutStream': 'Stream', 'InputStream': 'Stream', 'Menus': 'MenuSystem', 'Menu': 'MenuSystem', 'TextureMappings': 'TextureMappings',
    'Text': 'Text', 'Util': 'Util', 'Dir': 'Util', 'Particles': 'ParticleSystem',
    'ParticleSystem': 'ParticleSystem', 'Sound': 'Sound', 'SaveData': 'SaveData', 'SpellCircle': 'SpellCircle',
}

# names Ghidra was given that do not hold: (address -> why)
DISTRUST = {
    0x0200625c: 'a "neighbours" match with App_getMemUsed inside the Nitro SDK / C library',
}

HEAD = re.compile(r'^// ([0-9a-f]{8}) (\S+)\s*$')
CALL = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(')


class Func:
    def __init__(self, addr, name):
        self.addr, self.name = addr, name
        self.lines = []
        self.size = 0
        self.cls = None          # the class its name says
        self.file = None
        self.how = ''
        self.brew = ''
        self.calls = set()
        self.callers = set()


def read_funcs(path):
    funcs, cur = [], None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = HEAD.match(line)
            if m:
                cur = Func(int(m.group(1), 16), m.group(2))
                funcs.append(cur)
            elif cur:
                cur.lines.append(line.rstrip('\n'))
    funcs.sort(key=lambda f: f.addr)
    for a, b in zip(funcs, funcs[1:]):
        a.size = b.addr - a.addr
    return funcs


def read_matches(path):
    """matches.txt: ds address, ds name, brew address, brew name, method, score"""
    out = {}
    if os.path.exists(path):
        for line in open(path, encoding='utf-8', errors='replace'):
            p = line.rstrip('\n').split('\t')
            if len(p) >= 4:
                out[int(p[0], 16)] = '%s 0x%s' % (p[3], p[2])
    return out


def class_of(name):
    if re.match(r'^(FUN|thunk_FUN|LAB|SUB)_', name):
        return None
    head = name.split('_')[0]
    return FILE_OF_CLASS.get(head)


def segment(funcs):
    named = [f for f in funcs if f.cls]
    if not named:
        return
    first = min(f.addr for f in named)
    last = max(f.addr for f in named)
    game = [f for f in funcs if first <= f.addr <= last]
    for f in funcs:
        if f.addr < first:
            f.file, f.how = 'sdk/head', 'before the game code'
        elif f.addr > last:
            f.file, f.how = 'sdk/tail', 'after the game code'

    # a lone name whose neighbours on both sides agree on another class is taken as a wrong guess
    known = [i for i, f in enumerate(game) if f.cls]
    for k in range(1, len(known) - 1):
        p, c, n = game[known[k - 1]], game[known[k]], game[known[k + 1]]
        if p.cls == n.cls != c.cls:
            c.how = 'name says %s, among %s' % (c.cls, p.cls)
            c.cls_named, c.cls = c.cls, None
    known = [i for i, f in enumerate(game) if f.cls]
    for i in known:
        game[i].file = game[i].cls
        if not game[i].how:
            game[i].how = 'name'

    def votes(f):
        v = collections.Counter()
        for o in list(f.calls) + list(f.callers):
            if o.cls:
                v[o.cls] += 1
        return v

    for a, b in zip(known, known[1:]):
        gap = game[a + 1:b]
        if not gap:
            continue
        A, B = game[a].cls, game[b].cls
        if A == B:
            for f in gap:
                f.file, f.how = A, 'between two %s' % A
            continue
        # the cut k: gap[:k] in A, gap[k:] in B, with the most agreeing calls
        vs = [votes(f) for f in gap]
        best, bestk = None, len(gap) // 2
        for k in range(len(gap) + 1):
            s = sum(v[A] for v in vs[:k]) + sum(v[B] for v in vs[k:])
            if best is None or s > best:
                best, bestk = s, k
        for i, f in enumerate(gap):
            f.file = A if i < bestk else B
            f.how = 'cut %s|%s by calls' % (A, B) if best else 'cut %s|%s (no calls, middle)' % (A, B)


def restored_text(addr):
    p = os.path.join(ROOT, 'restored', '%08x.c' % addr)
    if os.path.exists(p):
        return open(p, encoding='utf-8-sig').read().rstrip('\n')
    return None


DEF = re.compile(r'^(?!\s|/|\*|#|static const)[\w\s\*]*?\b([A-Za-z_]\w*)\s*\(', re.M)


def restored_name(addr):
    """the name a restored function was given (its definition in restored/<address>.c)"""
    r = restored_text(addr)
    if r:
        m = DEF.search(r)
        if m:
            return m.group(1)
    return None


def write_tree(funcs, out):
    for f in funcs:
        n = restored_name(f.addr)
        if n and n != f.name:
            f.ghidra, f.name = f.name, n
    os.makedirs(os.path.join(out, 'map'), exist_ok=True)
    os.makedirs(os.path.join(out, 'src', 'sdk'), exist_ok=True)
    with open(os.path.join(out, 'map', 'functions.tsv'), 'w', encoding='utf-8', newline='\n') as t:
        t.write('address\tsize\tfile\tname\tbrew\tfile_from\trestored\n')
        for f in funcs:
            t.write('%08x\t%x\t%s\t%s\t%s\t%s\t%s\n' % (f.addr, f.size, f.file, f.name, f.brew, f.how,
                                                     'yes' if restored_text(f.addr) else ''))
    files = collections.OrderedDict()
    for f in funcs:
        files.setdefault(f.file, []).append(f)
    # the address ranges of the files, in link order; a file in more than one range is to be checked
    segs = []
    for f in funcs:
        if segs and segs[-1][0] == f.file:
            segs[-1][2].append(f)
        else:
            segs.append((f.file, f.addr, [f]))
    count = collections.Counter(s[0] for s in segs)
    with open(os.path.join(out, 'map', 'files.tsv'), 'w', encoding='utf-8', newline='\n') as t:
        t.write('file\tstart\tend\tfunctions\trestored\tnote\n')
        for name, start, fs in segs:
            t.write('%s\t%08x\t%08x\t%d\t%d\t%s\n' % (name, start, fs[-1].addr + fs[-1].size, len(fs),
                    sum(1 for f in fs if restored_text(f.addr)),
                    'in %d ranges' % count[name] if count[name] > 1 and not name.startswith('sdk') else ''))
    for name, fs in files.items():
        path = os.path.join(out, 'src', name + '.c')
        with open(path, 'w', encoding='utf-8', newline='\n') as o:
            o.write('/*\n * %s.c -- Orcs & Elves DS (YOEE) arm9, 0x%08x..0x%08x, %d functions.\n'
                    % (name.split('/')[-1], fs[0].addr, fs[-1].addr + fs[-1].size, len(fs)))
            o.write(' * Generated by tools/build_tree.py: restored functions come from restored/<address>.c,\n'
                    ' * the others are Ghidra\'s decompilation, kept in #if 0 until they are restored.\n */\n'
                    '#include "oeds.h"\n\n')
            for f in fs:
                o.write('/* 0x%08x (0x%x bytes) %s' % (f.addr, f.size, f.name))
                if getattr(f, 'ghidra', None):
                    o.write(' (Ghidra: %s)' % f.ghidra)
                if f.brew:
                    o.write('\n * BREW: %s' % f.brew)
                o.write('\n * file: %s */\n' % f.how)
                r = restored_text(f.addr)
                if r:
                    o.write(r + '\n\n')
                else:
                    body = '\n'.join(l for l in f.lines).strip('\n')
                    o.write('#if 0 /* raw */\n%s\n#endif\n\n' % body)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--decomp', default=os.path.join(ROOT, '..', 'arm9_usa.c'))
    ap.add_argument('--matches', default=os.path.join(ROOT, '..', 'matches.txt'))
    ap.add_argument('--out', default=ROOT)
    a = ap.parse_args()

    funcs = read_funcs(a.decomp)
    by_name = {f.name: f for f in funcs}       # (Ghidra's names: the calls in the raw text use them)
    # a restored function's name is the one that holds: it decides the file too
    for f in funcs:
        n = restored_name(f.addr)
        if n and n != f.name:
            f.ghidra, f.name = f.name, n
    matches = read_matches(a.matches)
    for f in funcs:
        f.cls = class_of(f.name) if f.addr not in DISTRUST else None
        f.brew = matches.get(f.addr, '') if f.addr not in DISTRUST and not getattr(f, 'ghidra', None) else ''
        for line in f.lines:
            for m in CALL.finditer(line):
                o = by_name.get(m.group(1))
                if o and o is not f:
                    f.calls.add(o)
                    o.callers.add(f)
    segment(funcs)
    write_tree(funcs, a.out)
    files = collections.Counter(f.file for f in funcs)
    print('%d functions, %d files' % (len(funcs), len(files)))
    for name, n in sorted(files.items(), key=lambda x: min(f.addr for f in funcs if f.file == x[0])):
        print('  %-22s %4d' % (name, n))


if __name__ == '__main__':
    main()
