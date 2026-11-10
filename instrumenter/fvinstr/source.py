# source.py - The source text as bytes, plus the list of text edits that are applied to it.

import re

from .errors import ToolError

# The source bytes with helpers for scanning them.
class Source:
    def __init__(self, data):
        self.data = data

    # Position of the next non-blank character.
    def skip_ws(self, pos):
        d = self.data
        n = len(d)
        while pos < n:
            c = d[pos:pos + 1]
            if c in (b" ", b"\t", b"\r", b"\n", b"\f", b"\v"):
                pos += 1
            elif d.startswith(b"//", pos):
                nl = d.find(b"\n", pos)
                pos = n if nl < 0 else nl + 1
            elif d.startswith(b"/*", pos):
                close = d.find(b"*/", pos + 2)
                pos = n if close < 0 else close + 2
            else:
                break
        return pos

    # Position of the parenthesis that closes the one at pos.
    def match_paren(self, pos):
        d = self.data
        if d[pos:pos + 1] != b"(":
            raise ToolError("expected '(' at offset %d" % pos)
        depth = 0
        n = len(d)
        while pos < n:
            c = d[pos:pos + 1]
            if c == b"(":
                depth += 1
            elif c == b")":
                depth -= 1
                if depth == 0:
                    return pos
            elif c in (b'"', b"'"):
                quote = c
                pos += 1
                while pos < n and d[pos:pos + 1] != quote:
                    if d[pos:pos + 1] == b"\\":
                        pos += 1
                    pos += 1
            elif d.startswith(b"//", pos) or d.startswith(b"/*", pos):
                pos = self.skip_ws(pos) - 1
            pos += 1
        raise ToolError("unbalanced parenthesis")

    # Offset just after the last top-level #include.
    def top_level_include_end(self):
        depth = 0
        last = None
        pos = 0
        d = self.data
        in_block_comment = False
        for raw in d.split(b"\n"):
            end = pos + len(raw)
            line = raw.strip()
            if in_block_comment:
                if b"*/" in line:
                    in_block_comment = False
            elif line.startswith(b"/*") and b"*/" not in line:
                in_block_comment = True
            elif line.startswith(b"#"):
                word = line[1:].strip().split(b" ")[0].split(b"\t")[0]
                word = re.split(rb"[<\"(]", word)[0]
                if word in (b"if", b"ifdef", b"ifndef"):
                    depth += 1
                elif word == b"endif":
                    depth -= 1
                elif word == b"include" and depth == 0:
                    last = end
                    if raw.endswith(b"\r"):
                        last = end - 1
            pos = end + 1
        return last

# Turns text into a C++ string literal.
def c_literal(raw):
    text = raw.decode("utf-8", errors="replace")
    text = re.sub(r"\s+", " ", text).strip()
    text = "".join(ch if 32 <= ord(ch) < 127 else "?" for ch in text)
    if len(text) > 90:
        text = text[:87] + "..."
    text = text.replace("\\", "\\\\").replace('"', '\\"').replace("??", "?\\?")
    return '"%s"' % text

CLOSE, OPEN, REPLACE = 0, 1, 2

# A list of insertions and replacements applied in one pass.
class Edits:
    def __init__(self):
        self.items = []
        self.seq = 0

    # Records an edit.
    def add(self, start, end, kind, text):
        self.seq += 1
        order = -self.seq if kind == CLOSE else self.seq
        self.items.append((start, kind, order, end, text))

    # Inserts text that opens a construct.
    def open(self, pos, text):
        self.add(pos, pos, OPEN, text)

    # Inserts text that closes a construct.
    def close(self, pos, text):
        self.add(pos, pos, CLOSE, text)

    # Replaces the bytes from start to end.
    def replace(self, start, end, text):
        self.add(start, end, REPLACE, text)

    # Returns the data with the edits applied.
    def apply(self, data, lo=0, hi=None, only_inside=False):
        hi = len(data) if hi is None else hi
        out = []
        cur = lo
        for start, _kind, _order, end, text in sorted(self.items):
            if only_inside and (start < lo or end > hi):
                continue
            if start < cur:
                continue
            if start > hi:
                break
            out.append(data[cur:start])
            out.append(text.encode("utf-8"))
            cur = max(cur, end)
        out.append(data[cur:hi])
        return b"".join(out)
