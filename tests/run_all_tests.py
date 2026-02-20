#!/usr/bin/env python3
"""
run_all_tests.py - Run all TeenAstro math library unit tests.

Usage:
    python tests/run_all_tests.py               # run all suites
    python tests/run_all_tests.py test_la3       # run one suite
    python tests/run_all_tests.py test_la3 test_coord   # run selected suites

Requires PlatformIO CLI (pio) on PATH.
"""

import subprocess
import sys
import os
import time

# Discover the tests/ directory (where this script lives)
TESTS_DIR = os.path.dirname(os.path.abspath(__file__))

# All available test suites (subdirectories under tests/test/)
ALL_SUITES = ["test_la3", "test_coord", "test_coordconv"]


def run_suite(suite_name: str) -> bool:
    """Run a single test suite via 'pio test'. Returns True on success."""
    print(f"\n{'='*60}")
    print(f"  Running: {suite_name}")
    print(f"{'='*60}\n")

    cmd = ["pio", "test", "-d", TESTS_DIR, "-e", "native", "--filter", suite_name]
    result = subprocess.run(cmd, cwd=TESTS_DIR)
    return result.returncode == 0


def main():
    # Determine which suites to run
    if len(sys.argv) > 1:
        suites = sys.argv[1:]
        for s in suites:
            if s not in ALL_SUITES:
                print(f"Unknown suite: {s}")
                print(f"Available: {', '.join(ALL_SUITES)}")
                sys.exit(1)
    else:
        suites = ALL_SUITES

    print(f"TeenAstro Math Library Unit Tests")
    print(f"Suites to run: {', '.join(suites)}")

    results = {}
    start = time.time()

    for suite in suites:
        results[suite] = run_suite(suite)

    elapsed = time.time() - start

    # Summary
    print(f"\n{'='*60}")
    print(f"  SUMMARY  ({elapsed:.1f}s)")
    print(f"{'='*60}")
    all_pass = True
    for suite, passed in results.items():
        status = "PASS" if passed else "FAIL"
        icon = "+" if passed else "!"
        print(f"  [{icon}] {suite:30s} {status}")
        if not passed:
            all_pass = False
    print(f"{'='*60}")

    if all_pass:
        print(f"\nAll {len(suites)} suite(s) passed.")
    else:
        failed = [s for s, p in results.items() if not p]
        print(f"\n{len(failed)} suite(s) FAILED: {', '.join(failed)}")

    sys.exit(0 if all_pass else 1)


if __name__ == "__main__":
    main()
