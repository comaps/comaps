#!/usr/bin/env python3
"""Generate cherry-pick tracking markdown from a git commit range.

Usage:
  python3 cherry_picking_issue.py <from-tag>..<to-tag>
  python3 cherry_picking_issue.py <from-tag>..<to-tag> -o output.md

Requires gh CLI for PR lookup.
"""

import argparse
import json
import subprocess
import sys
from collections import OrderedDict

REPO = "organicmaps/organicmaps"
BASE = f"https://github.com/{REPO}"

pr_cache = {}


def gh_api(endpoint, jq=None):
    cmd = ["gh", "api", endpoint]
    if jq:
        cmd += ["--jq", jq]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        return result.stdout.strip() if result.returncode == 0 else ""
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return ""


def get_pr_number(commit_hash):
    return gh_api(f"repos/{REPO}/commits/{commit_hash}/pulls", ".[0].number")


def get_pr_info(pr_number):
    if pr_number not in pr_cache:
        raw = gh_api(f"repos/{REPO}/pulls/{pr_number}")
        if raw:
            data = json.loads(raw)
            pr_cache[pr_number] = {
                "title": data.get("title", ""),
            }
        else:
            pr_cache[pr_number] = {"title": ""}
    return pr_cache[pr_number]


def git_log(commit_range):
    result = subprocess.run(
        ["git", "log", "--oneline", "--reverse", commit_range],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"git log failed: {result.stderr.strip()}", file=sys.stderr)
        sys.exit(1)
    return result.stdout.strip().splitlines()


def format_plain(commits, commit_range, out):
    """Inline links without bullet points. Blank lines between multi-commit PR groups."""
    groups = OrderedDict()
    for c in commits:
        key = c["pr"] or f"_no_pr_{c['hash']}"
        groups.setdefault(key, []).append(c)

    first = True
    prev_multi = False
    print(f"`git log --oneline --reverse {commit_range}`", file=out)
    print("", file=out)
    print("**Not processed yet**", file=out)
    print("", file=out)
    for key, group in groups.items():
        pr_number = group[0]["pr"]
        is_multi = len(group) > 1
        if not first and (prev_multi or is_multi):
            print("", file=out)
        if pr_number:
            info = get_pr_info(pr_number)
            title = info["title"] or group[0]["desc"]
            if len(group) == 1:
                c = group[0]
                desc = c["desc"]
                pr_link = f"[#{pr_number}]({BASE}/pull/{pr_number})"
                if f"(#{pr_number})" in desc:
                    desc = desc.replace(f"(#{pr_number})", f"{pr_link}")
                else:
                    desc += f" {pr_link}"
                print(f"`{c['hash']}` {desc}", file=out)
            else:
                print(f"PR: {title} [#{pr_number}]({BASE}/pull/{pr_number})", file=out)
                for c in group:
                    print(f"`{c['hash']}` {c['desc']}", file=out)
        else:
            for c in group:
                clink = f"[↗]({BASE}/commit/{c['hash']})"
                print(f"`{c['hash']}` {c['desc']} {clink}", file=out)
        prev_multi = is_multi
        first = False
    for section in [
        "Already done in Comaps",
        "Blocked due to closed source map generator",
        "Not wanted",
        "Not relevant (OM internal)",
    ]:
        print(f"\n---\n**{section}**\n", file=out)


def check_gh_cli():
    try:
        subprocess.run(["gh", "--version"], capture_output=True, timeout=5)
    except FileNotFoundError:
        print("Error: 'gh' CLI is not installed. Install it from https://cli.github.com/", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="Generate cherry-pick tracking markdown")
    parser.add_argument("range", help="Git commit range, e.g. tag1..tag2")
    parser.add_argument("-o", "--output", help="Output file (default: stdout)")
    args = parser.parse_args()

    check_gh_cli()

    lines = git_log(args.range)

    commits = []
    for i, line in enumerate(lines):
        sha, _, desc = line.partition(" ")
        print(f"  [{i + 1}/{len(lines)}] looking up {sha} {desc}", file=sys.stderr)
        pr = get_pr_number(sha)
        commits.append({"hash": sha, "desc": desc, "pr": pr})

    if args.output:
        with open(args.output, "w") as out:
            format_plain(commits, args.range, out)
        print(f"Wrote {len(commits)} commits to {args.output}", file=sys.stderr)
    else:
        format_plain(commits, args.range, sys.stdout)


if __name__ == "__main__":
    main()