---
name: exploring-codebase
description: Map a whole repo (every file, every branch) into docs/explore.md for /taking-notes. Use when asked to explore, map or get up to speed on a codebase, or before /taking-notes.
---
# Exploring a codebase
Goal: `docs/explore.md`, a map of the whole repo (≤ ~15k tokens) that /taking-notes quizzes the developer from.
You plan and merge; subagents read. This keeps your context small however big the repo is.
## 1. Inventory (facts only, no file contents)
From the repo root:
```bash
git fetch --all --prune 2>/dev/null
D=$(git symbolic-ref --short refs/remotes/origin/HEAD 2>/dev/null | sed 's@^origin/@@'); D=${D:-main}
# files and lines per top-level folder
git ls-files -z | xargs -0 wc -l 2>/dev/null | awk '$2!="total"{split($2,p,"/"); d=(p[2]?p[1]:"."); f[d]++; l[d]+=$1} END{for(d in f) printf "%6d %7d %s\n", f[d], l[d], d}' | sort -k2 -rn
# branches with commits not on the default branch
for b in $(git for-each-ref --format='%(refname:short)' refs/heads refs/remotes | grep -v -e '/HEAD$' -e '^origin$'); do
  n=$(git rev-list --count $D..$b 2>/dev/null); [ "${n:-0}" -gt 0 ] && echo "$b +$n $(git diff --shortstat $D...$b)"
done
git log --oneline -n 40
```
Ignore vendored and generated files (node_modules, lockfiles, build output, binaries).
## 2. Slice
- One slice per folder. Split big folders until each slice is under ~8k lines.
- One slice per branch that has its own commits. That slice covers the diff only.
- List merged or empty branches without reading them.
## 3. Read (subagents, in parallel)
Spawn readers with model: sonnet; use haiku for config, docs and CI slices.
Give each subagent its slice and this brief:
> Read these paths. Write `docs/.explore/<slice>.md`, at most ~1.5k tokens: purpose, key files, entry points, how it connects to other parts, notable patterns, and unknowns marked `unconfirmed:` or `guess:`. Cite paths (file:line where useful). For a branch: what it changes, why (from the commits), and its state compared to the default branch. Reply with the file path only.
Without a subagent tool, do the slices yourself one at a time. Write each section file, then rely on the files, not your memory.
## 4. Merge
Read the section files and write `docs/explore.md`. Remove duplicates and compress to fit the cap. Number the unknowns U1… across the whole file. Delete `docs/.explore/`. Re-running overwrites.
```
# Explore: <repo>
_<date> · default branch <D> · <n> files_
## Overview & stack
## Layout
## Modules
## Entry points & data flow
## Branches
## History
## Unknowns
- U1 (unconfirmed) … — `path`
- U2 (guess) … — `path`
```
Cite paths throughout. /taking-notes uses them to check answers.
## 5. Report
Tell the user the file count, branch count, and number of unknowns, and suggest /taking-notes. Don't edit any code.
