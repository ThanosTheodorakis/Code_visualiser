# ast_nodes.py - Small helpers for reading Clang syntax tree nodes.



# Flags describing where the visitor is (raw region, lambda, template, ...).
class Ctx:
    def __init__(self, **kw):
        self.raw = False
        self.in_lambda = False
        self.in_template = False
        self.nologic = False
        self.noio = False
        self.__dict__.update(kw)

    # Copy of the context with some flags changed.
    def but(self, **kw):
        c = Ctx(**self.__dict__)
        c.__dict__.update(kw)
        return c

# Child nodes that are real syntax nodes.
def kids(node):
    return [c for c in node.get("inner", ()) if isinstance(c, dict) and "kind" in c]

# Like kids, but keeps None in place of children that are not syntax nodes.
def all_kids(node):
    return [c if (isinstance(c, dict) and "kind" in c) else None
            for c in node.get("inner", ())]

# Skips implicit casts and temporaries to reach the real expression.
def strip_implicit(node):
    while node is not None and node["kind"] in ("ImplicitCastExpr", "ExprWithCleanups",
                                                "MaterializeTemporaryExpr",
                                                "CXXBindTemporaryExpr", "ConstantExpr"):
        sub = kids(node)
        if not sub:
            break
        node = sub[0]
    return node

# The expression's type as written by Clang.
def type_name(node):
    t = node.get("type") or {}
    return (t.get("desugaredQualType") or t.get("qualType") or "").strip()

# True for node kinds that are statements.
def is_stmt_kind(kind):
    return kind.endswith("Stmt") or kind in ("CXXCatchStmt",)

# True if a lambda appears anywhere inside the node.
def has_lambda(node):
    if node.get("kind") == "LambdaExpr":
        return True
    return any(has_lambda(child) for child in all_kids(node) if child is not None)
