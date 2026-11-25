---
name: hello-world
description: Use when Catlyn types /hello-world or asks for a Hello World C++ file. Practice skill for learning skill structure.
disable-model-invocation: true
argument-hint: "[path, default hello.cpp]"
allowed-tools: Bash(bash ${CLAUDE_SKILL_DIR}/scripts/make_hello.sh *)
---

# Hello World

Creates a C++ file that prints "Hello world".

## Steps

1. Target path is `$ARGUMENTS`. If empty, use `hello.cpp` in the current directory.
2. Run: `bash ${CLAUDE_SKILL_DIR}/scripts/make_hello.sh <path>`
3. Report the path in one line, plus the command to compile it:
   `g++ -std=c++20 <path> -o hello && ./hello`

The script refuses to write `main.cpp` or to overwrite an existing file. If it refuses, tell Catlyn why and ask for another path. Do not write the file by hand to get around it.
