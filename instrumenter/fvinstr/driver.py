# driver.py - Instruments whole files and parses the command line.

import argparse
import os
import re
import sys

from .clang_runner import Annotator, find_clang, norm_path, run_clang
from .constants import FOOTER_NAME, HEADER_NAME, USER_MAIN
from .errors import ToolError
from .instrumenter import Instrumenter

ENTRY_TEMPLATE = """
#include "{footer}"

int fvUserProgram()
{{
{body}
}}
"""

# True if the source defines a macro containing do ... while.
def has_do_while_macro(data):
    for match in re.finditer(rb"^[ \t]*#[ \t]*define[^\n]*(?:\\\r?\n[^\n]*)*", data, re.M):
        text = match.group(0)
        if re.search(rb"\bdo\b.*\bwhile\b", text, re.S):
            return True
    return False

# Instruments one source file and writes the result.
def instrument(source_path, output_path, clang, std, extra, header_dir, verbose):
    data = open(source_path, "rb").read()
    if data.startswith(b"\xef\xbb\xbf"):
        raise ToolError("%s starts with a UTF-8 byte order mark; save it without BOM." % source_path)
    if re.search(rb'#\s*include\s*[<"][^>"]*Instrument(End)?\.h', data):
        raise ToolError("%s already includes Instrument.h; it looks instrumented already." % source_path)
    source_abs = os.path.abspath(source_path)
    elements = run_clang(clang, source_abs, extra, std)
    header = header_dir + "/" + HEADER_NAME if header_dir else HEADER_NAME
    footer = header_dir + "/" + FOOTER_NAME if header_dir else FOOTER_NAME
    tool = Instrumenter(data, header, footer)
    sys.setrecursionlimit(200000)
    main_norm = norm_path(source_abs)
    for element, file_state in elements:
        Annotator(main_norm, file_state).annotate(element)
        tool.note_bitfields(element)
        tool.visit_decl(element, False)

    include_end = tool.src.top_level_include_end()
    text = '#include "%s"' % header
    if has_do_while_macro(data):
        text = "#define FV_NO_WHILE\n" + text
        tool.warn("a #define uses do/while, which the while macro of Instrument.h would break; "
                  "plain while loops are not animated in this file")
    if include_end is None:
        tool.edits.open(0, text + "\n")
    else:
        tool.edits.open(include_end, "\n" + text)
    body = tool.edits.apply(data)

    if not body.endswith(b"\n"):
        body += b"\n"
    if tool.has_main:
        if tool.main_takes_args:
            call = ('    char fvName[] = "program";\n'
                    "    char* fvArgv[] = { fvName, nullptr };\n"
                    "    return %s(1, fvArgv);" % USER_MAIN)
        else:
            call = "    return %s();" % USER_MAIN
        body += ENTRY_TEMPLATE.format(footer=footer, body=call).encode("utf-8")
    else:
        body += ('\n#include "%s"\n' % footer).encode("utf-8")
        tool.warn("no main() in this file: no fvUserProgram() was generated. "
                  "Instrument the file that has main, or call your own entry from the visualiser.")

    if data.count(b"\r\n") > data.count(b"\n") // 2:
        body = body.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    with open(output_path, "wb") as out:
        newline = "\r\n" if b"\r\n" in body else "\n"
        out.write(("// Instrumented by fv_instrument.py from %s%s" % (os.path.basename(source_path), newline)).encode("utf-8"))
        out.write(body)
    return tool

# Command line entry point; returns the exit code.
def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Instrument C++ source files for the Code Visualiser. For each "
                    "FILE a copy FILE_instrumented.EXT is written next to it. "
                    "Any argument that is not listed here (-I..., -D..., -std=...) "
                    "is handed to clang.")
    parser.add_argument("files", nargs="+", help="C++ source file(s) to instrument")
    parser.add_argument("--clang", help="path of clang / clang++ (default: search PATH, then the usual Windows places)")
    parser.add_argument("--std", default="c++17", help="language standard passed to clang (default c++17)")
    parser.add_argument("--header-dir", default="",
                        help='put this prefix in front of the Instrument.h includes, e.g. "include"')
    parser.add_argument("-o", "--output", help="output file (only with a single input file)")
    parser.add_argument("-v", "--verbose", action="store_true", help="print what was inserted")
    args, extra = parser.parse_known_args(argv)
    if args.output and len(args.files) != 1:
        parser.error("-o needs exactly one input file")
    try:
        clang = find_clang(args.clang)
        for path in args.files:
            if not os.path.isfile(path):
                raise ToolError("no such file: " + path)
            stem, ext = os.path.splitext(path)
            output = args.output or (stem + "_instrumented" + (ext or ".cpp"))
            tool = instrument(path, output, clang, args.std, extra, args.header_dir, args.verbose)
            s = tool.stats
            print("%s -> %s" % (path, output))
            print("  functions %d, statements %d, declarations %d, returns %d, input/output %d, "
                  "conditions %d, switch %d, do/while %d, if-with-init %d, left untouched %d" %
                  (s["functions"], s["statements"], s["declarations"], s["returns"], s["io"],
                   s["conditions"], s["switch"], s["do_while"], s["hoisted"], s["raw"]))
            for warning in tool.warnings:
                print("  warning: " + warning)
    except ToolError as error:
        print("error: %s" % error, file=sys.stderr)
        return 1
    return 0
