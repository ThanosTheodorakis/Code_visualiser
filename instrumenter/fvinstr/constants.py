# constants.py - Names of generated files and the sets of Clang node kinds the instrumenter treats alike.


HEADER_NAME = "Instrument.h"
FOOTER_NAME = "InstrumentEnd.h"
USER_MAIN = "fv_user_main"

FUNCTION_KINDS = {"FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl",
                  "CXXConversionDecl", "CXXDestructorDecl"}
CONTAINER_KINDS = {"NamespaceDecl", "LinkageSpecDecl", "CXXRecordDecl",
                   "ClassTemplateDecl", "FunctionTemplateDecl",
                   "ClassTemplateSpecializationDecl",
                   "ClassTemplatePartialSpecializationDecl"}
TEMPLATE_KINDS = {"ClassTemplateDecl", "FunctionTemplateDecl",
                  "ClassTemplateSpecializationDecl",
                  "ClassTemplatePartialSpecializationDecl"}
DECL_STMT_VARS = {"VarDecl", "DecompositionDecl"}

RETURN_TYPES = {
    "bool", "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "long", "unsigned long", "long long",
    "unsigned long long", "float", "double", "long double", "wchar_t",
    "char8_t", "char16_t", "char32_t", "const char *", "char *",
}

OUT_STREAMS = {"cout", "cerr", "clog"}
IN_STREAMS = {"cin"}
OUT_CALLS = {"printf", "puts", "putchar"}
IN_CALLS = {"scanf", "getchar", "gets"}
