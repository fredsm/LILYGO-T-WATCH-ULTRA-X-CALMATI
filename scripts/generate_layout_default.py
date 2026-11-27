"""Embed the single canonical SD layout into a C++ header for both builds."""
import argparse
from pathlib import Path


def generate(source: Path, destination: Path):
    data = source.read_bytes()
    if not data or len(data) > 4096 or b"\x00" in data:
        raise ValueError("layout_default.txt must be nonempty, <=4096 bytes, and contain no NUL")
    # Octal byte escapes preserve the exact file content, even CRLF and BOM.
    # Adjacent string literals keep compiler input lines short.
    chunks = ['"' + ''.join(f'\\{byte:03o}' for byte in data[i:i+32]) + '"'
              for i in range(0, len(data), 32)]
    header = "#pragma once\n// Generated from sd/Settings/layout_default.txt; do not edit.\n"
    header += "static constexpr char layout_embedded_text[] =\n" + "\n".join(chunks) + ";\n"
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text() != header:
        destination.write_text(header, encoding="ascii")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    generate(args.source, args.destination)
