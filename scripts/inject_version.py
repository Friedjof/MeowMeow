#!/usr/bin/env python3
import re
from pathlib import Path


def read_version():
    version_file = Path(__file__).resolve().parent.parent / "VERSION"
    try:
        content = version_file.read_text(encoding="utf-8").strip()
    except FileNotFoundError:
        return "v0.0.0-dev"

    version = re.sub(r"^/+\\s*", "", content)
    if not version:
        return "v0.0.0-dev"
    return version


def inject_version(version):
    header_path = Path(__file__).resolve().parent.parent / "include" / "version.h"
    if not header_path.exists():
        raise FileNotFoundError(f"Missing version header: {header_path}")

    content = header_path.read_text(encoding="utf-8")
    pattern = r'#define\\s+VERSION_STR\\s+"[^"]*"'
    replacement = f'#define VERSION_STR "{version}"'
    updated, count = re.subn(pattern, replacement, content)
    if count == 0:
        raise RuntimeError("VERSION_STR not found in include/version.h")

    header_path.write_text(updated, encoding="utf-8")


def main():
    version = read_version()
    inject_version(version)
    print(f"Updated include/version.h with {version}")


if __name__ == "__main__":
    main()
