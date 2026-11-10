# instrumenter.py - The Instrumenter class: walks the syntax tree and collects the edits for functions and declarations.

import re

from .ast_nodes import Ctx, all_kids, kids, type_name
from .constants import CONTAINER_KINDS, FUNCTION_KINDS, TEMPLATE_KINDS, USER_MAIN
from .expressions import ExpressionsMixin
from .source import Edits, Source
from .statements import StatementsMixin

# Walks the syntax tree and collects the text edits.
class Instrumenter(StatementsMixin, ExpressionsMixin):
    def __init__(self, source_bytes, header, footer):
        self.src = Source(source_bytes)
        self.edits = Edits()
        self.header = header
        self.footer = footer
        self.seen_functions = set()
        self.bitfields = set()
        self.has_main = False
        self.main_takes_args = 0
        self.stats = {"functions": 0, "statements": 0, "declarations": 0, "io": 0,
                      "returns": 0, "conditions": 0, "switch": 0, "do_while": 0,
                      "hoisted": 0, "raw": 0}
        self.warnings = []

    # Source bytes between two offsets.
    def slice(self, begin, end):
        return self.src.data[begin:end]

    # Source text between two offsets with the pending edits applied.
    def render(self, begin, end):
        return self.edits.apply(self.src.data, begin, end, only_inside=True).decode(
            "utf-8", errors="replace")

    # End offset of a statement including a trailing semicolon.
    def full_end(self, node):
        kind = node["kind"]
        rng = node.get("_r")
        if rng is None:
            return None
        if kind in ("IfStmt", "ForStmt", "WhileStmt", "CXXForRangeStmt", "SwitchStmt",
                    "CaseStmt", "DefaultStmt", "LabelStmt", "AttributedStmt"):
            sub = all_kids(node)
            last = sub[-1] if sub else None
            if last is not None:
                inner = self.full_end(last)
                if inner is not None:
                    return inner
            return rng[1]
        if kind in ("CompoundStmt", "DeclStmt", "NullStmt", "CXXTryStmt"):
            return rng[1]
        if kind == "DoStmt":
            end = self.src.skip_ws(rng[1])
            if self.src.data[end:end + 1] == b";":
                return end + 1
            return rng[1]
        pos = self.src.skip_ws(rng[1])
        if self.src.data[pos:pos + 1] == b";":
            return pos + 1
        return rng[1]

    # Remembers bit-field members, whose values cannot be passed by reference.
    def note_bitfields(self, node):
        if node.get("kind") == "FieldDecl" and node.get("isBitfield"):
            self.bitfields.add(node.get("id"))
        for child in node.get("inner", ()):
            if isinstance(child, dict):
                self.note_bitfields(child)

    # Records a warning for the final report.
    def warn(self, text):
        self.warnings.append(text)

    # Visits one declaration.
    def visit_decl(self, node, in_template):
        kind = node["kind"]
        if node.get("isImplicit"):
            return
        if kind in FUNCTION_KINDS:
            self.visit_function(node, in_template)
        elif kind in CONTAINER_KINDS:
            inside = in_template or kind in TEMPLATE_KINDS
            for child in kids(node):
                self.visit_decl(child, inside)

    # Adds entry and exit markers to a function.
    def visit_function(self, node, in_template):
        rng = node.get("_r")
        children = kids(node)
        body = children[-1] if children and children[-1]["kind"] in ("CompoundStmt",
                                                                    "CXXTryStmt") else None
        if rng is None or body is None:
            return
        if rng[0] in self.seen_functions:
            return
        self.seen_functions.add(rng[0])
        kind = node["kind"]
        qual = (node.get("type") or {}).get("qualType", "")
        raw = (kind == "CXXDestructorDecl" or node.get("constexpr") or node.get("constevalFunction")
               or re.search(r"noexcept(?!\s*\(\s*false)", qual) is not None
               or re.search(r"\bthrow\s*\(\s*\)", qual) is not None)
        if raw:
            self.edits.open(rng[0], '\n#include "%s"\n' % self.footer)
            self.edits.close(rng[1], '\n#include "%s"\n' % self.header)
            self.stats["raw"] += 1
            return
        if body["kind"] == "CXXTryStmt":
            self.warn("function-try-block in '%s' left without function marker" %
                      node.get("name", "?"))
            return
        ctx = Ctx(in_template=in_template)
        is_main = (kind == "FunctionDecl" and node.get("name") == "main"
                   and not in_template and node.get("_l") is not None)
        body_begin = body["_r"][0]
        if is_main:
            self.has_main = True
            params = [c for c in children if c["kind"] == "ParmVarDecl"]
            self.main_takes_args = 2 if len(params) >= 2 else 0
            self.edits.replace(node["_l"][0], node["_l"][0] + node["_l"][1], USER_MAIN)
            self.insert_in_body(body_begin, 'FUNCTION_SCOPE("main");')
            # The function returns int, so one that ends without a return gets "return 0;"; an existing
            # final return is left alone (a second one would be dead code).
            statements = kids(body)
            if not statements or statements[-1]["kind"] != "ReturnStmt":
                self.insert_before_closing_brace(body["_r"][1] - 1, "return 0;")
        else:
            names = []
            for c in children:
                if c["kind"] != "ParmVarDecl":
                    continue
                if not c.get("name") or c.get("isParameterPack"):
                    continue
                if "..." in type_name(c):
                    continue
                names.append(c["name"])
            self.insert_in_body(body_begin, "FV_FUNCTION(%s);" % ", ".join(names))
        self.stats["functions"] += 1
        self.visit_stmt(body, ctx)

    # Adds text just before a closing brace, on its own line if possible.
    def insert_before_closing_brace(self, brace, text):
        data = self.src.data
        if data[brace:brace + 1] != b"}":
            return
        line_start = data.rfind(b"\n", 0, brace) + 1
        before = data[line_start:brace]
        if before.strip() == b"":
            indent = re.match(rb"[ \t]*", before).group(0).decode()
            self.edits.open(line_start, indent + "    " + text + "\n")
        else:
            self.edits.open(brace, text + " ")

    # Adds text right after an opening brace.
    def insert_in_body(self, brace, text):
        data = self.src.data
        after = brace + 1
        probe = after
        while probe < len(data) and data[probe:probe + 1] in (b" ", b"\t"):
            probe += 1
        if data[probe:probe + 1] in (b"\n", b"\r"):
            line_start = data.rfind(b"\n", 0, brace) + 1
            indent = re.match(rb"[ \t]*", data[line_start:brace]).group(0).decode()
            self.edits.open(after, "\n" + indent + "    " + text)
        else:
            self.edits.open(after, " " + text)
