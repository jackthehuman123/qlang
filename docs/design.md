# QueryVM — Design Doc

Author: Hung Nguyen · Status: Draft · Last updated: <date>

## Problem
<Two sentences. What does this do, and who would care?>
- QueryVM filters rows out of large CSV files using a small query language, fast enough to be useful when spreadsheets stop being practical. It loads the data into memory in a columnar layout, compiles the query to bytecode, and scans in batches.

## Non-Goals
Explicitly out of scope:
- No joins between datasets
- No aggregations (COUNT, SUM, GROUP BY)
- No persistence — data is loaded into memory at startup
- Single-node, single-threaded (until Phase 10, if ever)
- Read-only

## Design
<The pipeline, as a diagram. ASCII is fine:>

  query text
     ↓  Lexer            → vector<Token>
     ↓  Parser (Pratt)   → AST (unique_ptr tree)
     ↓  Type checker     → typed AST, or an error
     ↓  Compiler         → Program { code, constants }
     ↓  VM               → row mask, 1024 rows at a time
  matching rows

## Key Decisions
| Decision | Chose | Alternatives considered | Why |
|---|---|---|---|
| Token text storage | string_view into source | owned std::string | Zero allocation per token; requires source to outlive the parse |
| AST representation | unique_ptr + virtual | std::variant + std::visit | Matches Crafting Interpreters; costs an indirect call per node, which the bytecode VM later removes |
| Parser style | Pratt / precedence climbing | recursive descent per level | Less code, easier to add operators |
| Batch size | 1024 rows | 256 / 4096 | Should stay L2-resident — will measure and confirm in Phase 6 |
| String columns | dictionary encoding | raw std::string compares | Turns string compares into int compares on low-cardinality columns |
| Correctness strategy | differential fuzzing vs tree-walk | hand-written tests only | Hand-written tests can't cover the query space; the oracle can |

## Risks / Open Questions
- <What might not work?>
- <What do you not know yet?>

## Milestones
<The table from Part 5, with real dates.>