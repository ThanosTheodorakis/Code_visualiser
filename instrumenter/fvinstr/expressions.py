# expressions.py - Instrumenter part for expressions: input and output calls, && and || and ?: conditions.

from .ast_nodes import kids, strip_implicit
from .constants import IN_CALLS, IN_STREAMS, OUT_CALLS, OUT_STREAMS

# Expression handling: input/output wrapping and condition markers.
class ExpressionsMixin:
    # Operator symbol of a binary or overloaded operator expression, else None.
    def op_name(self, node):
        kind = node["kind"]
        if kind == "BinaryOperator":
            return node.get("opcode")
        if kind == "CXXOperatorCallExpr":
            sub = kids(node)
            if sub:
                ref = self.find_ref(sub[0])
                if ref:
                    return ref.replace("operator", "")
        return None

    # Name of the first declaration the expression refers to, else None.
    def find_ref(self, node):
        if node is None:
            return None
        ref = node.get("referencedDecl")
        if isinstance(ref, dict) and ref.get("name"):
            return ref["name"]
        if node["kind"] in ("UnresolvedLookupExpr", "UnresolvedMemberExpr") and node.get("name"):
            return node["name"]
        for child in kids(node):
            found = self.find_ref(child)
            if found:
                return found
        return None

    # Follows a chain of << or >> back to the stream object.
    def stream_root(self, node, op):
        node = strip_implicit(node)
        if node is None:
            return None
        if node["kind"] in ("BinaryOperator", "CXXOperatorCallExpr") and self.op_name(node) == op:
            sub = kids(node)
            operands = sub[1:] if node["kind"] == "CXXOperatorCallExpr" else sub
            if not operands:
                return None
            return self.stream_root(operands[0], op)
        if node["kind"] == "DeclRefExpr":
            ref = node.get("referencedDecl") or {}
            return ref.get("name")
        return None

    # Tells whether the node is console input, console output, or neither.
    def io_kind(self, node):
        core = strip_implicit(node)
        if core is None:
            return None
        kind = core["kind"]
        if kind in ("BinaryOperator", "CXXOperatorCallExpr"):
            op = self.op_name(core)
            if op == "<<" and self.stream_root(core, "<<") in OUT_STREAMS:
                return "out"
            if op == ">>" and self.stream_root(core, ">>") in IN_STREAMS:
                return "in"
        elif kind == "CXXMemberCallExpr":
            sub = kids(core)
            if sub and sub[0]["kind"] == "MemberExpr":
                obj = kids(sub[0])
                root = self.stream_root(obj[0], "?") if obj else None
                if root in OUT_STREAMS:
                    return "out"
                if root in IN_STREAMS:
                    return "in"
        elif kind == "CallExpr":
            name = self.find_ref(kids(core)[0]) if kids(core) else None
            if name in OUT_CALLS:
                return "out"
            if name in IN_CALLS:
                return "in"
            if name == "getline" and len(kids(core)) > 1:
                if self.stream_root(kids(core)[1], "?") in IN_STREAMS:
                    return "in"
        return None

    # Wraps an input or output expression in its marker.
    def wrap_io(self, node, kind):
        core = strip_implicit(node)
        rng = core.get("_r") or node.get("_r")
        if rng is None:
            return
        macro = "FV_INPUT" if kind == "in" else "FV_OUTPUT"
        self.edits.open(rng[0], macro + "(")
        self.edits.close(rng[1], ")")
        self.stats["io"] += 1

    # True for &&, ||, ! and parentheses around them.
    def is_logic(self, node):
        node = strip_implicit(node)
        if node is None:
            return False
        kind = node["kind"]
        if kind == "BinaryOperator" and node.get("opcode") in ("&&", "||"):
            return True
        if kind == "ParenExpr":
            sub = kids(node)
            return bool(sub) and self.is_logic(sub[0])
        if kind == "UnaryOperator" and node.get("opcode") == "!":
            sub = kids(node)
            return bool(sub) and self.is_logic(sub[0])
        return False

    # Collects the individual conditions of a combined condition.
    def collect_leaves(self, node, out):
        node = strip_implicit(node)
        kind = node["kind"]
        sub = kids(node)
        if kind == "BinaryOperator" and node.get("opcode") in ("&&", "||"):
            self.collect_leaves(sub[0], out)
            self.collect_leaves(sub[1], out)
        elif kind in ("ParenExpr", "UnaryOperator") and sub and self.is_logic(sub[0]) \
                and (kind == "ParenExpr" or node.get("opcode") == "!"):
            self.collect_leaves(sub[0], out)
        else:
            out.append(node)

    # Wraps each part of a combined condition in FV_COND.
    def wrap_conditions(self, cond, ctx):
        leaves = []
        self.collect_leaves(cond, leaves)
        for leaf in leaves:
            rng = leaf.get("_r")
            if rng is None:
                continue
            self.edits.open(rng[0], "FV_COND(")
            self.edits.close(rng[1], ")")
            self.stats["conditions"] += 1
        for leaf in leaves:
            self.visit_expr(leaf, ctx.but(nologic=True))

    # Visits an expression and applies the needed rewrites.
    def visit_expr(self, node, ctx):
        if node is None or "kind" not in node or ctx.raw:
            return
        kind = node["kind"]
        if not ctx.noio:
            io = self.io_kind(node)
            if io:
                self.wrap_io(node, io)
                return
        if kind == "LambdaExpr":
            sub = kids(node)
            body = sub[-1] if sub and sub[-1]["kind"] == "CompoundStmt" else None
            if body is not None:
                self.visit_stmt(body, ctx.but(in_lambda=True), "CompoundStmt")
            return
        if kind in ("UnaryExprOrTypeTraitExpr", "CXXNoexceptExpr", "CXXTypeidExpr",
                    "RequiresExpr", "ConstantExpr", "CXXDefaultArgExpr"):
            return
        if kind == "StmtExpr":
            for child in kids(node):
                self.visit_stmt(child, ctx, "CompoundStmt")
            return
        if kind == "BinaryOperator" and node.get("opcode") in ("&&", "||") and not ctx.nologic:
            self.wrap_conditions(node, ctx)
            return
        if kind == "ConditionalOperator" and not ctx.nologic:
            sub = kids(node)
            if len(sub) == 3:
                self.wrap_conditions(sub[0], ctx)
                self.visit_expr(sub[1], ctx)
                self.visit_expr(sub[2], ctx)
                return
        for child in kids(node):
            self.visit_expr(child, ctx)
