# instrumenter_test.py - Tests for instrumenter/fv_instrument.py.

import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "instrumenter", "fv_instrument.py")
EXAMPLES = os.path.join(ROOT, "instrumenter", "examples")
INCLUDE = os.path.join(ROOT, "include")
MARKERS = os.path.join(ROOT, "src", "Markers.cpp")

WRAPPER = "int fvUserProgram();\nint main() { return fvUserProgram(); }\n"

def have_clang():
    if shutil.which("clang") or shutil.which("clang++"):
        return True
    for path in (r"C:\Program Files\LLVM\bin\clang.exe",):
        if os.path.exists(path):
            return True
    return False

def find_cxx():
    for name in ("g++", "clang++", "c++"):
        path = shutil.which(name)
        if path:
            return path
    return None

def run_tool(*args):
    return subprocess.run([sys.executable, TOOL] + list(args),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True)

def read(path):
    with open(path, "r", newline="") as f:
        return f.read().replace("\r\n", "\n")

class InstrumenterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="cv_instr_")

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.work, ignore_errors=True)

    def instrument_text(self, name, code):
        source = os.path.join(self.work, name)
        with open(source, "w") as f:
            f.write(code)
        result = run_tool(source)
        self.assertEqual(result.returncode, 0, result.stderr)
        return read(os.path.join(self.work, name.replace(".cpp", "_instrumented.cpp")))

    def test_examples_match_checked_in_output(self):
        for name in ("sample", "test"):
            source = os.path.join(self.work, name + ".cpp")
            shutil.copy(os.path.join(EXAMPLES, name + ".cpp"), source)
            result = run_tool(source)
            self.assertEqual(result.returncode, 0, result.stderr)
            made = read(os.path.join(self.work, name + "_instrumented.cpp"))
            expected = read(os.path.join(EXAMPLES, name + "_instrumented.cpp"))
            self.assertEqual(made, expected, name + " no longer instruments to the checked-in file")

    def test_instrumented_programs_print_the_same(self):
        cxx = find_cxx()
        if not cxx:
            self.skipTest("no g++/clang++ found")
        wrapper = os.path.join(self.work, "wrapper.cpp")
        with open(wrapper, "w") as f:
            f.write(WRAPPER)

        for name, stdin in (("sample", "5\n"), ("test", "")):
            plain_exe = os.path.join(self.work, name + "_plain")
            fv_exe = os.path.join(self.work, name + "_fv")
            plain_src = os.path.join(EXAMPLES, name + ".cpp")
            fv_src = os.path.join(EXAMPLES, name + "_instrumented.cpp")

            build = subprocess.run([cxx, "-std=c++17", plain_src, "-o", plain_exe],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   universal_newlines=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            build = subprocess.run([cxx, "-std=c++17", "-I" + INCLUDE, fv_src, wrapper, MARKERS,
                                    "-pthread", "-o", fv_exe],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   universal_newlines=True)
            self.assertEqual(build.returncode, 0, build.stderr)

            a = subprocess.run([plain_exe], input=stdin, stdout=subprocess.PIPE,
                               universal_newlines=True, timeout=60)
            b = subprocess.run([fv_exe], input=stdin, stdout=subprocess.PIPE,
                               universal_newlines=True, timeout=60)
            self.assertEqual(a.stdout, b.stdout, name + ": output differs after instrumenting")
            self.assertEqual(a.returncode, b.returncode)

    def test_function_and_return(self):
        text = self.instrument_text("t_func.cpp",
            "int twice(int x) { return x * 2; }\nint main() { return twice(2); }\n")
        self.assertIn("FV_FUNCTION(x)", text)
        self.assertIn("FV_RETURN(x * 2)", text)
        self.assertIn("fvUserProgram", text)

    def test_conditions_with_and_or_and_question_mark_are_marked(self):

        text = self.instrument_text("t_cond.cpp",
            "int main() { int a = 1; int b = 2;\n"
            "if (a > 0 && b > 0) { a = 2; }\n"
            "int m = a > b ? a : b;\n"
            "return m; }\n")
        self.assertIn("FV_COND(a > 0)", text)
        self.assertIn("FV_COND(b > 0)", text)
        self.assertIn("FV_COND(a > b)", text)

    def test_do_while_is_marked(self):
        text = self.instrument_text("t_loop.cpp",
            "int main() { int s = 0;\n"
            "do { ++s; } while (s < 5);\n"
            "return s; }\n")
        self.assertIn("FV_DO_WHILE(s < 5)", text)

    def test_for_and_while_are_left_to_the_header(self):
        text = self.instrument_text("t_for.cpp",
            "int main() { int s = 0;\n"
            "for (int i = 0; i < 3; ++i) { s += i; }\n"
            "while (s > 100) { --s; }\n"
            "return s; }\n")
        self.assertIn("for (int i = 0; i < 3; ++i)", text)
        self.assertIn("while (s > 100)", text)
        self.assertIn('FV_STMT_AS("s += i", s += i)', text)
        self.assertIn('FV_STMT_AS("--s", --s)', text)

    def test_switch_is_marked(self):
        text = self.instrument_text("t_switch.cpp",
            "int main() { int k = 2; switch (k) { case 1: return 1; case 2: return 2; default: return 0; } }\n")
        self.assertIn("FV_SWITCH (k)", text)

    def test_input_and_output_are_marked(self):
        text = self.instrument_text("t_io.cpp",
            "#include <iostream>\nint main() { int n = 0; std::cin >> n; std::cout << n << std::endl; return 0; }\n")
        self.assertIn("FV_INPUT", text)
        self.assertIn("FV_OUTPUT", text)

    def test_instrumenting_twice_gives_the_same_result(self):
        code = "int f(int a) { if (a) { return 1; } return 2; }\nint main() { return f(1); }\n"
        first = self.instrument_text("t_twice.cpp", code)
        second = self.instrument_text("t_twice.cpp", code)
        self.assertEqual(first, second)

    def test_header_dir_option(self):
        source = os.path.join(self.work, "t_hdr.cpp")
        with open(source, "w") as f:
            f.write("int main() { return 0; }\n")
        result = run_tool(source, "--header-dir", "include")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('"include/Instrument.h"', read(os.path.join(self.work, "t_hdr_instrumented.cpp")))

    def test_output_option(self):
        source = os.path.join(self.work, "t_out.cpp")
        target = os.path.join(self.work, "chosen_name.cpp")
        with open(source, "w") as f:
            f.write("int main() { return 0; }\n")
        result = run_tool(source, "-o", target)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(os.path.isfile(target))

    def test_missing_file_is_an_error(self):
        result = run_tool(os.path.join(self.work, "no_such_file.cpp"))
        self.assertEqual(result.returncode, 1)
        self.assertIn("no such file", result.stderr)

    def test_output_with_two_files_is_refused(self):
        a = os.path.join(self.work, "a.cpp")
        b = os.path.join(self.work, "b.cpp")
        for p in (a, b):
            with open(p, "w") as f:
                f.write("int main() { return 0; }\n")
        result = run_tool(a, b, "-o", os.path.join(self.work, "x.cpp"))
        self.assertNotEqual(result.returncode, 0)

    def build_and_run(self, name, code):
        cxx = find_cxx()
        if not cxx:
            self.skipTest("no g++/clang++ found")
        text = self.instrument_text(name + ".cpp", code)
        base = os.path.join(self.work, name)
        with open(base + "_main.cpp", "w") as f:
            f.write(WRAPPER)
        exe = base + "_exe"
        build = subprocess.run([cxx, "-std=c++17", "-I" + INCLUDE, base + "_instrumented.cpp",
                                base + "_main.cpp", MARKERS, "-pthread", "-o", exe],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               universal_newlines=True)
        self.assertEqual(build.returncode, 0, build.stderr)
        run = subprocess.run([exe], stdout=subprocess.PIPE, universal_newlines=True, timeout=60)
        return text, run

    def test_main_without_return_still_returns_zero(self):
        text, run = self.build_and_run("t_noreturn",
            "#include <iostream>\nint main() {\n    std::cout << \"hi\\n\";\n}\n")
        self.assertEqual(run.returncode, 0)
        self.assertEqual(run.stdout, "hi\n")
        self.assertIn("return 0;", text)

    def test_main_with_final_return_gets_no_second_return(self):
        text, run = self.build_and_run("t_finalreturn",
            "#include <iostream>\nint main() {\n    std::cout << \"hi\\n\";\n    return 3;\n}\n")
        self.assertEqual(run.returncode, 3)
        self.assertNotIn("return 0;", text)

    def test_do_while_inside_a_macro_compiles(self):
        text, run = self.build_and_run("t_macro_do",
            "#include <iostream>\n#define ONCE do { std::cout << \"once\\n\"; } while (0)\n"
            "int main() { ONCE; int i = 0; do { ++i; } while (i < 3); std::cout << i << \"\\n\"; }\n")
        self.assertEqual(run.stdout, "once\n3\n")
        self.assertIn("#define FV_NO_WHILE", text)

    def test_constexpr_lambda_initialiser_is_left_alone(self):
        text, run = self.build_and_run("t_constexpr_lambda",
            "#include <iostream>\nint main() {\n"
            "    constexpr int sq = [] { int s = 0; for (int i = 0; i < 4; ++i) s += i; return s; }();\n"
            "    std::cout << sq << \"\\n\";\n}\n")
        self.assertEqual(run.stdout, "6\n")

    def test_streamed_argument_of_its_own_operator_does_not_recurse(self):
        text, run = self.build_and_run("t_stream_arg",
            "#include <iostream>\nstruct V { int x; };\n"
            "std::ostream& operator<<(std::ostream& os, const V& v) { return os << v.x; }\n"
            "int main() { V v{4}; std::cout << v << \"\\n\"; }\n")
        self.assertEqual(run.stdout, "4\n")

    def test_bytes_that_are_not_utf8_are_kept(self):
        source = os.path.join(self.work, "t_latin1.cpp")
        with open(source, "wb") as f:
            f.write(b'// caf\xe9\n#include <iostream>\nint main() { std::cout << "caf\xe9"; }\n')
        result = run_tool(source)
        self.assertEqual(result.returncode, 0, result.stderr)
        with open(os.path.join(self.work, "t_latin1_instrumented.cpp"), "rb") as f:
            data = f.read()
        self.assertIn(b'"caf\xe9"', data)
        self.assertNotIn(b"\xef\xbf\xbd", data)

if __name__ == "__main__":
    if not have_clang():
        print("instrumenter_test: skipped (clang not found)")
        sys.exit(77)
    unittest.main(verbosity=2)
