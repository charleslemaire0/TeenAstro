#!/usr/bin/env python3
"""
Generate CommandReplyLength.h (C++) and lx200_reply_lengths.dart from
command_reply_lengths.json.

Run from repo root: python scripts/generate_reply_lengths.py

Source: libraries/TeenAstroCommandDef/data/command_reply_lengths.json
"""
import json
import os

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JSON_PATH = os.path.join(REPO_ROOT, "libraries", "TeenAstroCommandDef", "data", "command_reply_lengths.json")
H_OUT = os.path.join(REPO_ROOT, "libraries", "TeenAstroCommandDef", "src", "CommandReplyLength.h")
DART_OUT = os.path.join(REPO_ROOT, "teenastro_app", "lib", "models", "lx200_reply_lengths.dart")


def align_to_power2(n: int) -> int:
    """Round up to next power of 2 (4, 8, 16, 32, ...). Minimum 4."""
    if n <= 4:
        return 4
    p = 4
    while p < n:
        p *= 2
    return p


def main():
    with open(JSON_PATH, encoding="utf-8") as f:
        data = json.load(f)

    # Filter out non-string keys and ensure we have command -> length
    raw_lengths = {k: v for k, v in data.items() if isinstance(k, str) and isinstance(v, int) and not k.startswith("_")}
    # Align each length up to next power of 2 (4, 8, 16, 32, ...)
    lengths = {k: align_to_power2(v) for k, v in raw_lengths.items()}

    # Generate C++ header
    h_lines = [
        "/*",
        " * CommandReplyLength.h - Generated from command_reply_lengths.json",
        " * Do not edit manually. Run: python scripts/generate_reply_lengths.py",
        " */",
        "#pragma once",
        "",
        '#include "CommandEnums.h"',
        '#include "CommandMeta.h"',
        "",
        "/// Expected reply payload length (chars before '#'). Returns -1 for variable length, 0 for no reply, 1 for short.",
        "inline int getExpectedReplyLength(const char* command)",
        "{",
        "  if (!command || command[0] != ':') return -1;",
        "  CMDREPLY r = getReplyType(command);",
        "  if (r == CMDR_NO) return 0;",
        "  if (r == CMDR_SHORT || r == CMDR_SHORT_BOOL) return 1;",
        "  if (r != CMDR_LONG) return -1;",
        "  // Fixed-length CMDR_LONG commands:",
        "",
    ]
    for cmd, length in sorted(lengths.items()):
        # Escape for C string
        cmd_escaped = cmd.replace("\\", "\\\\").replace('"', '\\"')
        h_lines.append(f'  if (strcmp(command, "{cmd_escaped}") == 0) return {length};')
    h_lines.extend([
        "  return -1;",
        "}",
        "",
    ])

    with open(H_OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(h_lines))

    # Generate Dart file
    dart_lines = [
        "// Generated from command_reply_lengths.json.",
        "// Do not edit manually. Run: python scripts/generate_reply_lengths.py",
        "",
        "/// Expected reply payload length (chars before '#'). Returns null for variable length.",
        "int? getExpectedReplyLength(String cmd) {",
        "  switch (cmd) {",
    ]
    for cmd, length in sorted(lengths.items()):
        dart_lines.append(f'    case "{cmd}": return {length};')
    dart_lines.extend([
        "    default: return null;",
        "  }",
        "}",
        "",
        "/// Constants for commonly used fixed-length commands.",
        "abstract class LX200ReplyLength {",
    ])
    # Only emit constants that are used by the app for EXACT length validation (base64/binary).
    # Use raw (unaligned) lengths - these must match actual reply size.
    used_constants = {":GXAS#": "gxas", ":GXCS#": "gxcs", ":GXI#": "gxi", ":FA#": "faConfig", ":Fa#": "faState"}
    for cmd in sorted(used_constants.keys()):
        const_name = used_constants[cmd]
        length = raw_lengths.get(cmd, 0)
        dart_lines.append(f"  static const int {const_name} = {length};")
    dart_lines.extend([
        "}",
        "",
    ])

    with open(DART_OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(dart_lines))

    print(f"Generated {H_OUT}")
    print(f"Generated {DART_OUT}")


if __name__ == "__main__":
    main()
