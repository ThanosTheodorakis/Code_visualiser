# Instrumenter

`fv_instrument.py` takes a normal C++ file and writes a copy with the markers already in it,
so Code_visualiser can show it. You do not change your program. The original file is never
touched.

This folder is separate from the rest of the project. The visualiser only uses the file the
tool writes.

## What you need

* Python 3.8 or newer (on `PATH`)
* Clang (any recent LLVM). The tool runs `clang -fsyntax-only -Xclang -ast-dump=json` and
  reads the result, so nothing has to be built. If Clang is not found, install it with
  `winget install LLVM.LLVM`, or pass its path: `--clang "C:/Program Files/LLVM/bin/clang++.exe"`.

## How to use it

1. Instrument your file (run this in the project folder):

       python instrumenter\fv_instrument.py path\to\prog.cpp

   This writes `prog_instrumented.cpp` next to `prog.cpp`. If your program needs include
   paths or defines, add them to the command, the same as when you compile it:

       python instrumenter\fv_instrument.py prog.cpp -Iinclude -DDEBUG --std c++20

   Other options: `-o file` (output name), `-v` (more messages), `--header-dir` (prefix for
   the `Instrument.h` include).

2. Build the visualiser with the new file:

       cmake -S app -B build -DFV_USER_PROGRAM=C:/path/to/prog_instrumented.cpp
       cmake --build build --config Release

3. Run `Code_visualiser.exe` from the `app` folder. Type input in the
   console window.

If you change your program, run the tool again and rebuild. Do not edit the `_instrumented`
file by hand, because it is replaced each time.

Examples are in `instrumenter/examples`: `sample.cpp` with its output `sample_instrumented.cpp`,
and `test.cpp`, which reads two numbers and does some work with loops and branches.

## What the tool adds

| In your program | In the copy |
|---|---|
| After the last `#include` | `#include "Instrument.h"`, which turns `if`, `for` and `while` into markers |
| End of file | `#include "InstrumentEnd.h"` and a function `fvUserProgram()` that runs your `main` |
| `int main(...)` | Renamed `fv_user_main` (it is still shown as `main`) |
| Every function with a body | `FV_FUNCTION(params);` as the first line |
| `return value;` (numbers, `bool`, `char`, text) | `return FV_RETURN(value);` |
| Statements like `total += x;` | `FV_STMT_AS("total += x", total += x);` |
| Local variables with a value | `FV_DECL_AS("int n = f()");` before the declaration |
| `cin >> x`, `getline`, `scanf`, `getchar` | `FV_INPUT(...)` |
| `cout << x`, `printf`, `puts`, `putchar` | `FV_OUTPUT(...)` |
| `switch (x)` | `FV_SWITCH (x)` |
| `do { } while (c);` | `FV_DO_WHILE(c) { }` |
| Parts of `&&`, `||` and `?:` | `FV_COND(part)` |
| `if (init; cond)` or `if (T v = f())` | The declaration is moved just before the `if`, inside `{ }` |

The conditions of `for`, `while` and `do/while` do not get `FV_COND`, because that would draw
one fork for every iteration.

## What the tool leaves alone

* Code inside macros and the inactive side of `#if`.
* `constexpr` and `consteval` functions, destructors and `noexcept` functions (a marker can
  not run at compile time, and a stop request must not be thrown out of `noexcept` code).
* Global variables, default arguments, `static_assert` and other places that need constant
  values.
* `return` inside lambdas and templates, and returns of other types (for example vectors or
  your own classes). They run normally but the value is not drawn.

## Good to know

* Your file must compile. If Clang finds errors, the tool stops and shows them.
* The program runs inside the visualiser, so `exit()`, `abort()` or a crash closes the window.
* Programs with several threads are not designed for and may look mixed up.
* Closing the window stops the program at its next marker.

## How the markers work

The tool only inserts markers. The drawing is done by the headers in `include/`.

* **`if`, `for`, `while`.** `Instrument.h` redefines these keywords as macros, so they report
  themselves without any change to the code. `InstrumentEnd.h` turns the macros off again.
  Standard library headers must be included before `Instrument.h`.
* **Guards.** Each marker that starts a block creates a small object. Its destructor reports
  the end, so the end is reported also after `break`, `continue`, `return` or an exception.
* **Everything else** (functions, `switch`, `do/while`, `?:`, `&&`, `||`, input and output)
  cannot be done with keyword macros, so the tool inserts the macros from `Markers.h`. You
  can also write them by hand.
* **Cost.** Every marker is a function call and a short lock, so an instrumented program is
  slower. The visualiser also slows it down on purpose to match the animation.

Where the code is: `include/Instrument.h`, `include/InstrumentEnd.h`, `include/Markers.h`,
`src/Markers.cpp`, and the test `tests/demo_test.cpp`.
