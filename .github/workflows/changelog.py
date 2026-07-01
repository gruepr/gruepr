#!/usr/bin/env python3
"""gruepr changelog automation.

Three subcommands, used by the release workflows:

  extract-done   Parse the "// DONE:" block in main.cpp and print it as
                 Markdown bullets (used to seed the draft release notes).
  prepend        Prepend a "## vX.Y.Z - DATE" section to CHANGELOG.md.
  clear-done     Empty the "// DONE:" block in main.cpp.

The DONE block is everything between the "// DONE:" line and the "// TO DO:"
line in main.cpp. Inside it:
  - "//  - text"      becomes a top-level bullet
  - "//      - text"  (deeper indent) becomes a nested bullet
  - "//  SOMETHING:"  (a non-bullet line ending in ":") becomes a **bold** sub-header
"""

import argparse
import re
import sys

DONE_RE = re.compile(r'^//\s*DONE:\s*$')
TODO_RE = re.compile(r'^//\s*TO\s*DO:\s*$')


def read(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


def write(path, text):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(text)


def normalize_version(v):
    """13 -> 13.0.0, 13.2 -> 13.2.0, 13.2.1 -> 13.2.1 (leave 3+ parts alone)."""
    v = v.strip().lstrip('vV')
    parts = v.split('.')
    if len(parts) == 1:
        parts += ['0', '0']
    elif len(parts) == 2:
        parts += ['0']
    return '.'.join(parts)


def done_lines(main_path):
    """Return the raw lines between '// DONE:' and '// TO DO:' (exclusive)."""
    lines = read(main_path).splitlines()
    start = end = None
    for i, line in enumerate(lines):
        if start is None and DONE_RE.match(line):
            start = i + 1
        elif start is not None and TODO_RE.match(line):
            end = i
            break
    if start is None:
        sys.exit("error: no '// DONE:' marker found in " + main_path)
    if end is None:
        sys.exit("error: no '// TO DO:' marker found after '// DONE:' in " + main_path)
    return lines[start:end]


def extract_done(main_path):
    """Turn the DONE block into Markdown bullets."""
    bullets = []
    indents = []  # leading-space counts of bullets, to find the base indent
    parsed = []   # (kind, indent, text)

    for raw in done_lines(main_path):
        body = raw[2:] if raw.startswith('//') else raw  # drop leading '//'
        stripped = body.strip()
        if not stripped:
            continue
        indent = len(body) - len(body.lstrip(' '))
        if stripped.startswith('- '):
            text = stripped[2:].strip()
            parsed.append(('bullet', indent, text))
            indents.append(indent)
        elif stripped.startswith('-'):
            text = stripped[1:].strip()
            parsed.append(('bullet', indent, text))
            indents.append(indent)
        elif stripped.endswith(':'):
            parsed.append(('header', indent, stripped[:-1].strip()))
        else:
            parsed.append(('bullet', indent, stripped))
            indents.append(indent)

    base = min(indents) if indents else 0
    for kind, indent, text in parsed:
        if kind == 'header':
            bullets.append('')
            bullets.append('**{}**'.format(text))
        else:
            level = max(0, (indent - base) // 4)
            bullets.append('{}- {}'.format('  ' * level, text))

    # tidy leading blank line if the first item was a header
    out = '\n'.join(bullets).strip('\n')
    return out


def prepend(changelog_path, version, date, notes):
    version = normalize_version(version)
    notes = notes.strip('\n')
    section = '## v{} — {}\n\n{}\n\n'.format(version, date, notes)

    text = read(changelog_path)
    lines = text.splitlines(keepends=True)
    # insert immediately before the first "## " heading (after the preamble)
    insert_at = None
    for i, line in enumerate(lines):
        if line.startswith('## '):
            insert_at = i
            break
    if insert_at is None:
        # no existing version sections; append after whatever preamble exists
        new_text = text.rstrip('\n') + '\n\n' + section
    else:
        new_text = ''.join(lines[:insert_at]) + section + ''.join(lines[insert_at:])
    write(changelog_path, new_text)


def clear_done(main_path):
    """Remove the item lines inside the DONE block, leaving '// DONE:' empty."""
    lines = read(main_path).splitlines(keepends=True)
    out = []
    in_done = False
    for line in lines:
        stripped = line.rstrip('\n')
        if DONE_RE.match(stripped):
            out.append(line)          # keep the "// DONE:" marker
            out.append('//\n')        # one blank comment line for spacing
            in_done = True
            continue
        if in_done and TODO_RE.match(stripped):
            in_done = False
            out.append(line)          # keep the "// TO DO:" marker
            continue
        if in_done:
            continue                  # drop everything between the markers
        out.append(line)
    write(main_path, ''.join(out))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)

    p1 = sub.add_parser('extract-done')
    p1.add_argument('--main', default='main.cpp')

    p2 = sub.add_parser('prepend')
    p2.add_argument('--version', required=True)
    p2.add_argument('--date', required=True)
    p2.add_argument('--notes-file', required=True)
    p2.add_argument('--changelog', default='CHANGELOG.md')

    p3 = sub.add_parser('clear-done')
    p3.add_argument('--main', default='main.cpp')

    args = ap.parse_args()
    if args.cmd == 'extract-done':
        print(extract_done(args.main))
    elif args.cmd == 'prepend':
        prepend(args.changelog, args.version, args.date, read(args.notes_file))
    elif args.cmd == 'clear-done':
        clear_done(args.main)


if __name__ == '__main__':
    main()
