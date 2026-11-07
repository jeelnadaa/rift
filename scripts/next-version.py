#!/usr/bin/env python3
"""Print the next Rift release version, computed from git tags.

Rules (see AGENTS.md: versions are never hand-edited):
  * no v* tag yet                         -> 1.0.0 (or the VERSION floor, if higher)
  * latest tag vX.Y.Z                      -> X.Y.(Z+1)
  * VERSION file says "A.B" and A.B > X.Y  -> A.B.0   (bump major/minor by editing VERSION)

Usage: python3 scripts/next-version.py            (reads `git tag`)
       python3 scripts/next-version.py v1.2.3 ... (tags given explicitly, for tests)
"""
import os
import re
import subprocess
import sys

TAG = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")


def parse_tags(tags):
    out = []
    for t in tags:
        m = TAG.match(t.strip())
        if m:
            out.append(tuple(int(x) for x in m.groups()))
    return sorted(out)


def read_floor(path):
    try:
        with open(path, encoding="utf-8") as f:
            m = re.match(r"^\s*(\d+)\.(\d+)", f.read())
        return (int(m.group(1)), int(m.group(2))) if m else None
    except OSError:
        return None


def next_version(tags, floor=None):
    versions = parse_tags(tags)
    if not versions:
        base = (1, 0)
        if floor and floor > base:
            base = floor
        return "%d.%d.0" % base
    major, minor, patch = versions[-1]
    if floor and floor > (major, minor):
        return "%d.%d.0" % floor
    return "%d.%d.%d" % (major, minor, patch + 1)


def main(argv):
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if len(argv) > 1:
        tags = argv[1:]
    else:
        tags = subprocess.run(["git", "tag", "--list", "v*"], cwd=root, capture_output=True, text=True,
                              check=True).stdout.split()
    print(next_version(tags, read_floor(os.path.join(root, "VERSION"))))


if __name__ == "__main__":
    main(sys.argv)
