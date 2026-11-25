---
name: taking-notes
description: Quiz the developer on a codebase from docs/explore.md; record each question, their answer and the actual answer in docs/ans-notes.md. Use for /taking-notes or to check repo understanding.
---
# Taking notes
Goal: you and the developer share the same understanding of the codebase. They come away with `docs/ans-notes.md`, a reference they'll study later. Write it for a human skimming it, not for agents.
## Setup
- Read `docs/explore.md` once. If it's missing, tell the user to run /exploring-codebase first, and stop.
- If `docs/ans-notes.md` exists, run only `grep '^## Q' docs/ans-notes.md`. The headers give you the last Q number, the U IDs already asked, and past verdicts. Never read the entry bodies.
- Otherwise create it with `# Codebase notes` and a line giving today's date and a link to explore.md.
## Loop (one question per message)
Pick in this order: Unknowns not yet asked, then topics with past ❌, then gaps (entry points, data flow, modules, error paths).
1. **Ask.** Anchor the question to a concrete detail, e.g. "`api/server.ts` registers routes before the auth middleware. What happens to a request to /admin with no token?" Prefer how, why and what-if questions over trivia. Don't hint at the answer.
2. **Wait.** "Don't know" is a fine answer; record it as that.
3. **Check.** Use explore.md if it marks the point as confirmed. If not, `grep -n` for the symbol in the files it points to and read only that range (offset/limit), never whole files. If you're still unsure, say so in the actual answer.
4. **Reply.** In one message: the verdict, the actual answer in 1–4 sentences with file:line, then the next question.
5. **Append** the entry with a shell heredoc (`cat >> docs/ans-notes.md <<'EOF'`). Never Read, Edit or Write the whole notes file.
If the developer disputes an actual answer, re-check the code and fix that one entry with a targeted edit.
Entry format. The verdict goes in the header so grep can see it; leave out the U ID if the question isn't from Unknowns:
```
## Q3 · ❌ · Modules · U4
**Question:** …
**Your answer:** …
**Actual answer:** … (`src/db/pool.ts:42`)
```
Verdicts: ✅ match, ⚠️ partial, ❌ miss.
## Stop
Stop after ~15 questions this session, when nothing is left, or when the user says to stop. Close with this session's tally, its ❌ topics, and "Run /taking-notes again to continue."
## Rules
- Never edit explore.md or code.
- Keep a session under ~60k tokens.
