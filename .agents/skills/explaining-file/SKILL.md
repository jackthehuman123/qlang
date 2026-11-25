---
name: explain-file
description: Use when Catlyn types /explain-file with a source file path and wants a short explanation of what the file does and how it connects to the rest of the codebase.
disable-model-invocation: true
argument-hint: "<path to file>"
context: fork
agent: Explore
background: false
---

You are explaining one file of a codebase to its author, who has forgotten it.

File to explain: $ARGUMENTS

1. Read the file in full.
2. Find its connections: what it includes or imports, and which other files include it or call its functions. Use Grep. Read only the parts of those files you need.
3. If the file has tests, find them and note which behaviors they cover.

Return exactly this, and nothing else:

**Purpose:** one sentence.

**Key pieces:** 3 to 6 bullets, each naming a function, class or type with `file:line` and what it does in one line.

**Connections:**
- Uses: files this one depends on, one line each.
- Used by: files that depend on this one, one line each.

**Tested by:** test file and the behaviors it checks, or "no tests found."

**One thing to look at first:** the single function to read to understand this file, with `file:line`.
