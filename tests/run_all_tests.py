#!/usr/bin/env python3
"""
run_all_tests.py - Run all native PlatformIO unit tests, optionally emulator regression.

Usage:
    python tests/run_all_tests.py
                        # all discovered test_* suites under tests/test/
    python tests/run_all_tests.py --with-emulator
                        # same + GXAS meridian/flip regression (TCP 127.0.0.1:9997;
                        # start mainunit_emu first)
    python tests/run_all_tests.py test_la3
    python tests/run_all_tests.py test_la3 test_coord

Requires PlatformIO CLI (pio) on PATH.
"""

import subprocess
import sys
import os
import time

# Discover the tests/ directory (where this script lives)
TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(TESTS_DIR)


def discover_native_suites():
    """Subdirectories tests/test/test_* that contain a test_*.cpp (skip empty placeholders)."""
    test_root = os.path.join(TESTS_DIR, "test")
    if not os.path.isdir(test_root):
        return []
    names = []
    for name in sorted(os.listdir(test_root)):
        path = os.path.join(test_root, name)
        if not os.path.isdir(path) or not name.startswith("test_"):
            continue
        try:
            has_cpp = any(
                f.endswith(".cpp") and f.startswith("test_")
                for f in os.listdir(path)
            )
        except OSError:
            continue
        if has_cpp:
            names.append(name)
    return names


def run_suite(suite_name: str) -> bool:
    """Run a single test suite via 'pio test'. Returns True on success."""
    print(f"\n{'='*60}")
    print(f"  Running: {suite_name}")
    print(f"{'='*60}\n")

    cmd = ["pio", "test", "-d", TESTS_DIR, "-e", "native", "--filter", suite_name]
    result = subprocess.run(cmd, cwd=TESTS_DIR)
    return result.returncode == 0


def run_emulator_gxas_regression() -> bool:
    """
    Integration regression: TeenAstroEmulator mainunit_emu on TCP 9997.
    Does not write EEPROM (script default). Requires mainunit_emu running.
    """
    script = os.path.join(
        REPO_ROOT, "TeenAstroEmulator", "tools", "emu_flip_gxas_mimic_test.py"
    )
    if not os.path.isfile(script):
        print(f"Missing script: {script}")
        return False
    print(f"\n{'='*60}")
    print("  Emulator regression: emu_flip_gxas_mimic_test.py (127.0.0.1:9997)")
    print(f"{'='*60}\n")
    cmd = [sys.executable, "-u", script, "127.0.0.1", "9997"]
    result = subprocess.run(cmd, cwd=REPO_ROOT)
    return result.returncode == 0


def main():
    argv = [a for a in sys.argv[1:] if a != "--with-emulator"]
    with_emulator = "--with-emulator" in sys.argv[1:]

    all_suites = discover_native_suites()
    if not all_suites:
        print("No tests/test/test_* suites found.")
        sys.exit(1)

    if len(argv) > 0:
        suites = argv
        for s in suites:
            if s not in all_suites:
                print(f"Unknown suite: {s}")
                print(f"Available: {', '.join(all_suites)}")
                sys.exit(1)
    else:
        suites = all_suites

    print("TeenAstro native unit tests (PlatformIO Unity)")
    print(f"Suites to run: {', '.join(suites)}")

    results = {}
    start = time.time()

    for suite in suites:
        results[suite] = run_suite(suite)

    if with_emulator:
        results["emulator_gxas_flip"] = run_emulator_gxas_regression()

    elapsed = time.time() - start

    # Summary
    print(f"\n{'='*60}")
    print(f"  SUMMARY  ({elapsed:.1f}s)")
    print(f"{'='*60}")
    all_pass = True
    for name, passed in results.items():
        status = "PASS" if passed else "FAIL"
        icon = "+" if passed else "!"
        print(f"  [{icon}] {name:30s} {status}")
        if not passed:
            all_pass = False
    print(f"{'='*60}")

    if all_pass:
        print(f"\nAll {len(results)} run(s) passed.")
    else:
        failed = [s for s, p in results.items() if not p]
        print(f"\n{len(failed)} run(s) FAILED: {', '.join(failed)}")

    sys.exit(0 if all_pass else 1)


if __name__ == "__main__":
    main()
