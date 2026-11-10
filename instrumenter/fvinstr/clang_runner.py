# clang_runner.py - Finds and runs clang, reads its syntax tree and tags each node with its position in the file.

import os
import shutil
import subprocess
import threading

from .errors import ToolError

# Finds a working clang (given path, CLANG variable, or common names).
def find_clang(explicit):
    candidates = []
    if explicit:
        candidates.append(explicit)
    if os.environ.get("CLANG"):
        candidates.append(os.environ["CLANG"])
    candidates += ["clang++", "clang", "clang++-18", "clang++-17", "clang++-16",
                   "clang++-19", "clang++-20"]
    for name in candidates:
        path = shutil.which(name)
        if path:
            return path
    program_files = [os.environ.get("ProgramFiles", r"C:\Program Files"),
                     os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")]
    for base in program_files:
        for rel in (r"LLVM\bin\clang++.exe", r"LLVM\bin\clang.exe"):
            path = os.path.join(base, rel)
            if os.path.exists(path):
                return path
        vs_root = os.path.join(base, "Microsoft Visual Studio")
        if os.path.isdir(vs_root):
            for dirpath, _dirs, files in os.walk(vs_root):
                if "clang++.exe" in files and os.path.basename(dirpath) == "bin":
                    return os.path.join(dirpath, "clang++.exe")
                if dirpath.count(os.sep) - vs_root.count(os.sep) > 7:
                    _dirs[:] = []
    raise ToolError("clang was not found. Install LLVM/Clang, or pass --clang PATH "
                    "(or set the CLANG environment variable).")

# Normalises a path so file names compare equal.
def norm_path(path):
    return os.path.normcase(os.path.normpath(os.path.abspath(path)))

# Runs clang to get the JSON syntax tree of the source.
def run_clang(clang, source, extra_args, std):
    cmd = [clang, "-fsyntax-only", "-w", "-fno-color-diagnostics",
           "-fno-delayed-template-parsing", "-ferror-limit=0"]
    if not any(a.startswith("-std=") or a.startswith("/std:") for a in extra_args):
        cmd.append("-std=" + std)
    cmd += extra_args
    cmd += ["-Xclang", "-ast-dump=json", "-x", "c++", source]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, encoding="utf-8", errors="replace")
    errors = []
    reader = threading.Thread(target=lambda: errors.append(proc.stderr.read()))
    reader.start()
    elements = list(iter_main_elements(proc.stdout, source))
    proc.stdout.read()
    proc.stdout.close()
    reader.join()
    code = proc.wait()
    message = errors[0] if errors else ""
    if code != 0:
        raise ToolError("clang could not compile %s:\n%s" % (source, message.strip()))
    if not elements:
        raise ToolError("clang produced no declarations for %s (is the file empty, or is this "
                        "clang too old to write -ast-dump=json?)" % source)
    return elements

# Streams the top-level declarations that belong to the user file, skipping headers.
def iter_main_elements(stream, source):
    import json
    main = norm_path(source)
    cache = {}

    def is_main(name):
        if name not in cache:
            cache[name] = norm_path(name) == main
        return cache[name]

    state = None
    buf = None
    prev = ""
    for line in stream:
        if buf is None:
            if line.rstrip("\r\n") == "    {":
                buf = [line]
                prev = ""
            continue
        buf.append(line)
        stripped = line.strip()
        if line.rstrip("\r\n") in ("    },", "    }"):
            before = state
            touches_main = before is not None and is_main(before)
            last = None
            prior = ""
            for text in buf:
                if '"file":' in text:
                    if not prior.endswith('"includedFrom": {'):
                        value = text.split('"file":', 1)[1].strip().rstrip(",")
                        value = json.loads(value)
                        last = value
                        if is_main(value):
                            touches_main = True
                prior = text.strip()
            if touches_main:
                body = "".join(buf).rstrip().rstrip(",")
                yield json.loads(body), before
            if last is not None:
                state = last
            buf = None
        prev = stripped

# Adds byte ranges (_r) to syntax nodes, following Clang's compact location format.
class Annotator:

    def __init__(self, main, file_state):
        self.main = main
        self.file = file_state

    # Reads one location as (file, offset, token length).
    def bare(self, loc):
        if not loc:
            return None
        if "file" in loc:
            self.file = loc["file"]
        if "offset" not in loc:
            return None
        return (self.file, loc["offset"], loc.get("tokLen", 0))

    # Reads a location; returns "macro" if it comes from a macro expansion.
    def read(self, loc):
        if not loc:
            return None
        if "spellingLoc" in loc or "expansionLoc" in loc:
            self.bare(loc.get("spellingLoc"))
            self.bare(loc.get("expansionLoc"))
            return "macro"
        return self.bare(loc)

    # True if the location is in the user file.
    def is_main(self, info):
        return (info is not None and info != "macro" and info[0] is not None
                and norm_path(info[0]) == self.main)

    # Walks a node and its children and stores their byte ranges.
    def annotate(self, node):
        if not isinstance(node, dict) or "kind" not in node:
            return
        loc = self.read(node.get("loc"))
        rng = node.get("range") or {}
        begin = self.read(rng.get("begin"))
        end = self.read(rng.get("end"))
        node["_l"] = (loc[1], loc[2]) if self.is_main(loc) else None
        if self.is_main(begin) and self.is_main(end):
            node["_r"] = (begin[1], end[1] + end[2])
        else:
            node["_r"] = None
        for child in node.get("inner", ()):
            self.annotate(child)
