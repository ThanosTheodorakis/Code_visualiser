# statements.py - Instrumenter part for statements: if, switch, loops, returns and declarations.

import re

from .ast_nodes import all_kids, has_lambda, is_stmt_kind, kids, strip_implicit, type_name
from .constants import DECL_STMT_VARS, RETURN_TYPES
from .source import c_literal

# Statement handling: loops, branches, switches, returns and declarations.
class StatementsMixin:
    # Visits a statement and wraps it as its kind requires.
    def visit_stmt(self, node, ctx, parent=None):
        if node is None:
            return
        kind = node["kind"]
        if kind == "CompoundStmt":
            for child in kids(node):
                self.visit_stmt(child, ctx, "CompoundStmt")
        elif kind == "DeclStmt":
            self.visit_declstmt(node, ctx, mark=(parent == "CompoundStmt"))
        elif kind == "IfStmt":
            self.visit_if(node, ctx)
        elif kind == "ForStmt":
            sub = all_kids(node)
            if len(sub) == 5:
                init, _cv, cond, inc, body = sub
                if init is not None:
                    if init["kind"] == "DeclStmt":
                        self.visit_declstmt(init, ctx, mark=False)
                    else:
                        self.visit_expr(init, ctx.but(nologic=True))
                if cond is not None:
                    self.visit_expr(cond, ctx.but(nologic=True))
                if inc is not None:
                    self.visit_expr(inc, ctx.but(nologic=True))
                self.visit_stmt(body, ctx, "ForStmt")
        elif kind == "WhileStmt":
            sub = kids(node)
            if node.get("hasVar") and len(sub) == 3:
                self.visit_declstmt(sub[0], ctx, mark=False)
                self.visit_expr(sub[1], ctx.but(nologic=True))
                self.visit_stmt(sub[2], ctx, "WhileStmt")
            elif len(sub) == 2:
                self.visit_expr(sub[0], ctx.but(nologic=True))
                self.visit_stmt(sub[1], ctx, "WhileStmt")
        elif kind == "DoStmt":
            self.visit_do(node, ctx)
        elif kind == "CXXForRangeStmt":
            sub = all_kids(node)
            if sub:
                self.visit_stmt(sub[-1], ctx, "CXXForRangeStmt")
        elif kind == "SwitchStmt":
            self.visit_switch(node, ctx)
        elif kind in ("CaseStmt", "DefaultStmt", "LabelStmt", "AttributedStmt"):
            sub = all_kids(node)
            if sub and sub[-1] is not None:
                self.visit_stmt(sub[-1], ctx, kind)
        elif kind == "ReturnStmt":
            self.visit_return(node, ctx)
        elif kind == "CXXTryStmt":
            for child in kids(node):
                self.visit_stmt(child, ctx, "CXXTryStmt")
        elif kind == "CXXCatchStmt":
            sub = kids(node)
            if sub:
                self.visit_stmt(sub[-1], ctx, "CXXCatchStmt")
        elif is_stmt_kind(kind):
            return
        else:
            self.visit_expr_stmt(node, ctx)

    # Marks an expression statement.
    def visit_expr_stmt(self, node, ctx):
        rng = node.get("_r")
        if rng is None or ctx.raw:
            return
        io = self.io_kind(node)
        if io:
            self.wrap_io(node, io)
            return
        text = c_literal(self.slice(*rng))
        self.edits.open(rng[0], "FV_STMT_AS(%s, " % text)
        self.edits.close(rng[1], ")")
        self.stats["statements"] += 1
        self.visit_expr(node, ctx)

    # Marks a declaration statement.
    def visit_declstmt(self, node, ctx, mark):
        rng = node.get("_r")
        variables = [c for c in kids(node) if c["kind"] in DECL_STMT_VARS]
        has_init = False
        wrapped = False
        for var in variables:
            vr = var.get("_r")
            loc = var.get("_l")
            if vr is not None and loc is not None and vr[1] > loc[0] + loc[1]:
                has_init = True
            constant = bool(var.get("constexpr")) or "constinit" in str(var.get("storageClass", ""))
            sub = ctx.but(nologic=True, noio=True) if constant else ctx
            if constant and not ctx.raw and rng is not None and has_lambda(var):
                sub = sub.but(raw=True)
                if not wrapped:
                    self.edits.open(rng[0], '\n#include "%s"\n' % self.footer)
                    self.edits.close(rng[1], '\n#include "%s"\n' % self.header)
                    self.stats["raw"] += 1
                    wrapped = True
            for child in kids(var):
                self.visit_expr(child, sub)
        if mark and has_init and rng is not None and not ctx.raw:
            statics = any(v.get("storageClass") == "static" or v.get("constexpr")
                          or v.get("tls") for v in variables)
            if not statics:
                text = c_literal(self.slice(rng[0], rng[1] - 1 if self.slice(rng[1] - 1, rng[1]) == b";" else rng[1]))
                self.edits.open(rng[0], "FV_DECL_AS(%s); " % text)
                self.stats["declarations"] += 1

    # Wraps the returned value so it is reported.
    def visit_return(self, node, ctx):
        sub = kids(node)
        if not sub:
            return
        expr = sub[0]
        rng = expr.get("_r")
        if rng is not None and not ctx.raw and not ctx.in_lambda and not ctx.in_template:
            if self.returns_showable(expr, rng):
                self.edits.open(rng[0], "FV_RETURN(")
                self.edits.close(rng[1], ")")
                self.stats["returns"] += 1
        self.visit_expr(expr, ctx)

    # True if the returned value can be shown safely.
    def returns_showable(self, expr, rng):
        if self.slice(rng[0], rng[0] + 1) == b"{":
            return False
        core = strip_implicit(expr)
        if core is not None and core["kind"] == "MemberExpr" \
                and core.get("referencedMemberDecl") in self.bitfields:
            return False
        if core is not None and core["kind"] in ("InitListExpr", "CXXStdInitializerListExpr",
                                                  "CXXNullPtrLiteralExpr"):
            return False
        t = type_name(expr).replace("const ", "").replace("volatile ", "")
        t = re.sub(r"\s+", " ", t).strip()
        if t in RETURN_TYPES or t.replace("const ", "") in RETURN_TYPES:
            return True
        if re.fullmatch(r"(std::)?(__cxx11::)?basic_string<char.*>", t) or t in ("std::string", "string"):
            return True
        return False

    # Splits an if or switch into init, condition variable, condition and the rest.
    def hoist_parts(self, node, sub):
        index = 0
        init = condvar = None
        if node.get("hasInit"):
            init = sub[index]
            index += 1
        if node.get("hasVar"):
            condvar = sub[index]
            index += 1
        cond = sub[index]
        return init, condvar, cond, sub[index + 1:]

    # Moves the init and condition variable of an if or switch out in front of it.
    def hoist(self, node, init, condvar, cond):
        rng = node["_r"]
        keyword = 2 if node["kind"] == "IfStmt" else 6
        lparen = self.src.skip_ws(rng[0] + keyword)
        rparen = self.src.match_paren(lparen)
        pieces = []
        if init is not None:
            ir = init["_r"]
            text = self.render(*ir)
            pieces.append(text if init["kind"] == "DeclStmt" else text + ";")
        if condvar is not None:
            vr = condvar["_r"]
            pieces.append(self.render(*vr) + ";")
            var = [c for c in kids(condvar) if c["kind"] in DECL_STMT_VARS][0]
            cond_text = var["name"]
        else:
            cond_text = self.render(*cond["_r"])
        self.edits.open(rng[0], "{ " + " ".join(pieces) + " ")
        self.edits.replace(lparen, rparen + 1, "(" + cond_text + ")")
        self.edits.close(self.full_end(node), " }")
        self.stats["hoisted"] += 1

    # Marks an if statement and its branches.
    def visit_if(self, node, ctx):
        sub = kids(node)
        if not sub or node.get("_r") is None:
            return
        init, condvar, cond, rest = self.hoist_parts(node, sub)
        constexpr = bool(node.get("isConstexpr"))
        if init is not None:
            if init["kind"] == "DeclStmt":
                self.visit_declstmt(init, ctx, mark=False)
            else:
                self.visit_expr(init, ctx)
        if condvar is not None:
            self.visit_declstmt(condvar, ctx, mark=False)
        if not constexpr:
            self.visit_expr(cond, ctx)
        if (init is not None or condvar is not None) and not constexpr and not ctx.raw:
            if (init is None or init.get("_r")) and (condvar is None or condvar.get("_r")) \
                    and cond.get("_r"):
                self.hoist(node, init, condvar, cond)
            else:
                self.warn("an if with an init-statement was left as it was")
        for branch in rest:
            self.visit_stmt(branch, ctx, "IfStmt")

    # Marks a switch statement and its cases.
    def visit_switch(self, node, ctx):
        sub = kids(node)
        rng = node.get("_r")
        if not sub or rng is None:
            return
        init, condvar, cond, rest = self.hoist_parts(node, sub)
        if init is not None:
            if init["kind"] == "DeclStmt":
                self.visit_declstmt(init, ctx, mark=False)
            else:
                self.visit_expr(init, ctx)
        if condvar is not None:
            self.visit_declstmt(condvar, ctx, mark=False)
        self.visit_expr(cond, ctx)
        if not ctx.raw:
            self.edits.replace(rng[0], rng[0] + 6, "FV_SWITCH")
            self.stats["switch"] += 1
            if init is not None or condvar is not None:
                if cond.get("_r"):
                    self.hoist(node, init, condvar, cond)
        for child in rest:
            self.visit_stmt(child, ctx, "SwitchStmt")

    # Marks a do-while loop.
    def visit_do(self, node, ctx):
        sub = kids(node)
        rng = node.get("_r")
        if len(sub) != 2 or rng is None:
            return
        body, cond = sub
        self.visit_stmt(body, ctx, "DoStmt")
        self.visit_expr(cond, ctx.but(nologic=True))
        if ctx.raw or body.get("_r") is None or cond.get("_r") is None:
            return
        body_end = self.full_end(body)
        pos = self.src.skip_ws(body_end)
        if self.src.data[pos:pos + 5] != b"while":
            self.warn("could not find 'while' of a do/while near offset %d" % rng[0])
            return
        lparen = self.src.skip_ws(pos + 5)
        rparen = self.src.match_paren(lparen)
        semi = self.src.skip_ws(rparen + 1)
        if self.src.data[semi:semi + 1] != b";":
            self.warn("do/while without ';' near offset %d" % rng[0])
            return
        cond_text = self.render(*cond["_r"])
        self.edits.replace(rng[0], rng[0] + 2, "FV_DO_WHILE(%s) " % cond_text)
        self.edits.replace(pos, semi + 1, "")
        self.stats["do_while"] += 1
