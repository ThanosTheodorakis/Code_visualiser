#!/usr/bin/env python3
# fv_instrument.py - Command line entry that adds the visualiser markers to a C++ file (code is in fvinstr/).

import os
import sys
import threading

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from fvinstr.driver import main

if __name__ == "__main__":
    sys.setrecursionlimit(200000)
    for size in (512, 256, 64):
        try:
            threading.stack_size(size * 1024 * 1024)
            break
        except (ValueError, RuntimeError):
            continue
    result = []
    worker = threading.Thread(target=lambda: result.append(main()))
    worker.start()
    worker.join()
    sys.exit(result[0] if result else 1)
