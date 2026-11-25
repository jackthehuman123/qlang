#!/usr/bin/env bash
# Copies assets/hello.cpp to the target path.
# Usage: bash make_hello.sh [path]   (default: hello.cpp)
set -euo pipefail

target="${1:-hello.cpp}"
skill_dir="$(cd "$(dirname "$0")/.." && pwd)"

if [[ "$(basename "$target")" == "main.cpp" ]]; then
    echo "refused: won't write main.cpp" >&2
    exit 1
fi

if [[ -e "$target" ]]; then
    echo "refused: $target already exists" >&2
    exit 1
fi

mkdir -p "$(dirname "$target")"
cp "$skill_dir/assets/hello.cpp" "$target"
echo "created $target"
