#!/usr/bin/env bash
# agents-setup.sh — make a repo work for Claude Code AND agent-neutral tools.
#
#   Source of truth:  AGENTS.md            .agents/skills/
#   Claude adapters:  CLAUDE.md (@AGENTS.md)  .claude/skills -> ../.agents/skills
#
# Usage (run from anywhere inside the repo):
#   bash agents-setup.sh            team mode: changes meant to be committed
#   bash agents-setup.sh --local    local mode: for repos you don't own; nothing
#                                   shows up in git status
#   add --dry-run to either to print what would happen without changing anything
#
# Safe to run repeatedly. Never overwrites or deletes a file with content;
# stops before touching anything if moving skills would clash.
# Written for macOS's bash 3.2 as well as Linux.

set -euo pipefail

mode=team
dry=0
for arg in "$@"; do
    case "$arg" in
        --local)   mode=local ;;
        --dry-run) dry=1 ;;
        -h|--help) sed -n '2,16p' "$0"; exit 0 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -z "$root" ]; then
    echo "error: not inside a git repository" >&2
    exit 1
fi
cd "$root"
exclude="$(git rev-parse --git-path info/exclude)"   # works in worktrees too
linked=0

say()  { echo "  $*"; }
act()  { # act <description> <command...>
    local desc="$1"; shift
    if [ "$dry" -eq 1 ]; then echo "  would: $desc"; else "$@"; echo "  done:  $desc"; fi
}
write_file() { printf '%s\n' "$2" > "$1"; }
append_line() { printf '%s\n' "$2" >> "$1"; }
has_line() { [ -f "$1" ] && grep -qxF "$2" "$1"; }

# Add a path to .git/info/exclude (local mode) or .gitignore (team mode).
ignore() {
    local file="$1" pattern="$2"
    has_line "$file" "$pattern" && return 0
    act "add '$pattern' to $file" append_line "$file" "$pattern"
}

echo "repo: $root  (mode: $mode$( [ "$dry" -eq 1 ] && echo ', dry run'))"

# ---------------------------------------------------------------- 1. skills
echo "skills:"
if [ -L .claude/skills ]; then
    if [ -e .claude/skills ]; then
        say "ok: .claude/skills -> $(readlink .claude/skills)"
    else
        echo "error: .claude/skills is a broken link -> $(readlink .claude/skills)" >&2
        echo "       inspect it, then remove it with: rm .claude/skills" >&2
        exit 1
    fi

elif [ -d .claude/skills ]; then
    # Real folder: migrate its contents into .agents/skills, then link.
    shopt -s dotglob nullglob
    entries=(.claude/skills/*)
    shopt -u dotglob nullglob

    # Check everything BEFORE moving anything, so a clash leaves the repo untouched.
    to_move=()
    for e in ${entries[@]+"${entries[@]}"}; do
        name="$(basename "$e")"
        if [ "$name" = ".DS_Store" ]; then continue; fi
        if [ -L "$e" ] && [ "$name" = "skills" ]; then continue; fi   # misplaced link from an earlier ln -s
        if [ -e ".agents/skills/$name" ]; then
            echo "error: '$name' exists in both .claude/skills and .agents/skills." >&2
            echo "       Decide which to keep, delete the other, and run again. Nothing was changed." >&2
            exit 1
        fi
        to_move+=("$e")
    done

    act "create .agents/skills" mkdir -p .agents/skills
    for e in ${entries[@]+"${entries[@]}"}; do
        name="$(basename "$e")"
        if [ "$name" = ".DS_Store" ]; then act "remove $e" rm "$e"; fi
        if [ -L "$e" ] && [ "$name" = "skills" ]; then act "remove misplaced link $e" rm "$e"; fi
    done
    for e in ${to_move[@]+"${to_move[@]}"}; do
        act "move $e -> .agents/skills/" mv "$e" .agents/skills/
    done
    act "remove empty .claude/skills folder" rmdir .claude/skills
    act "link .claude/skills -> ../.agents/skills" ln -s ../.agents/skills .claude/skills; linked=1

elif [ -d .agents/skills ]; then
    act "create .claude" mkdir -p .claude
    act "link .claude/skills -> ../.agents/skills" ln -s ../.agents/skills .claude/skills; linked=1

elif [ "$mode" = team ]; then
    act "create .agents/skills" mkdir -p .agents/skills
    act "create .claude" mkdir -p .claude
    act "link .claude/skills -> ../.agents/skills" ln -s ../.agents/skills .claude/skills; linked=1

else
    say "no skills in this repo; nothing to link"
fi

if [ "$mode" = local ] && { [ -L .claude/skills ] || [ "$linked" -eq 1 ]; }; then
    ignore "$exclude" ".claude/skills"
fi

# ---------------------------------------------------------- 2. instructions
echo "instructions:"
imp="@AGENTS.md"
# Track state in variables so --dry-run reports what WOULD happen accurately.
agents=0;  [ -f AGENTS.md ] && agents=1
claude=0;  [ -f CLAUDE.md ] && claude=1
claude_imports=0; has_line CLAUDE.md "$imp" && claude_imports=1

if [ "$mode" = team ]; then
    if [ "$agents" -eq 0 ]; then
        if [ -s CLAUDE.md ] && [ "$claude_imports" -eq 0 ]; then
            # Existing Claude-only instructions become the neutral source of truth.
            act "move CLAUDE.md content into AGENTS.md" mv CLAUDE.md AGENTS.md
            claude=0
        else
            act "create AGENTS.md" write_file AGENTS.md "# Agent instructions"
        fi
    fi
    if [ "$claude" -eq 0 ]; then
        act "write CLAUDE.md containing '$imp'" write_file CLAUDE.md "$imp"
    elif [ "$claude_imports" -eq 0 ]; then
        act "add '$imp' to existing CLAUDE.md" append_line CLAUDE.md "$imp"
    else
        say "ok: CLAUDE.md imports AGENTS.md"
    fi
    ignore .gitignore "CLAUDE.local.md"
    ignore .gitignore ".claude/settings.local.json"

else  # local: touch nothing that git would show
    if [ "$agents" -eq 0 ]; then
        say "no AGENTS.md; nothing to import"
    elif [ "$claude_imports" -eq 1 ]; then
        say "ok: repo's CLAUDE.md already imports AGENTS.md"
    elif has_line CLAUDE.local.md "$imp"; then
        say "ok: CLAUDE.local.md already imports AGENTS.md"
        ignore "$exclude" "CLAUDE.local.md"
    else
        act "add '$imp' to CLAUDE.local.md" append_line CLAUDE.local.md "$imp"
        ignore "$exclude" "CLAUDE.local.md"
    fi
fi

# ------------------------------------------------------- 3. settings sanity
if [ -f .claude/settings.json ] && [ ! -s .claude/settings.json ]; then
    echo "settings:"
    act "fill empty .claude/settings.json with {} (empty file is invalid JSON)" \
        write_file .claude/settings.json "{}"
fi

echo "finished. Restart Claude Code so it picks up the changes."
