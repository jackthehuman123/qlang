# Build Notes

## Who does what
CMake      generates build files (commands + dependency graph). Then exits.
ninja/make decides which commands need running, runs them in parallel.
g++        preprocess, compile, invoke ld.
clangd     separate compiler, background only, for editor squiggles.
           Produces no files. Reads compile_commands.json.
           Can't catch link errors — it doesn't link.

## How C++ programs are built:
- `g++` is a C++ compiler that preprocesses, compiles, and invokes the linker (`ld`). It produces an executable; running it is a separate step.
- sources files + headers   --preprocesses--> translation unit (one per `.cpp` file)
- translation unit          --compiles--> object file `.o` (machine code + unresolved names)
- `.o` files + (`.a` libs)  --link--> executable

## Why linking exists
- Each `cpp` compiles in isolation and knows nothing about the others.
- Headers give it _declarations_ so it can compile.
- The linker suppliesthe _definitions_ so it can run.
- missing declaration -> compile error: 'foo' was not declared in this scope.
- missing definition -> link error: undefined reference to 'foo::dosomething()'.

## CMake
### The problems
Compiling by hand doesn't scale past a few files:

- Commands are prone to human errors.
- Changing a header might make (many) `.cpp` file(s) require rebuilding (or else they cause undefined behaviors, bugs, etc).
- Flags differ across compilers and platforms.

### The solution
CMake read `CMakeLists.txt` and generates the build files: g++ commands and a dependency graph. It also writes `compile_commands.json`, which clangd reads, so the editor and the build agree.

### Targets
A target is a thing that gets built:

- `add_library(qlang_core src/lexer.cpp)`   -> libqlang_core.a, no main(), not runnable
- `add_executable(qlang src/main.cpp)`      -> a runnable program

Every `target_*` command has the same shape:
`target_<property>( <which target> <PUBLIC|PRIVATE> <value> )`

### PUBLIC vs PRIVATE
When something links me, does it need this too?
- PUBLIC -> it does. Propagates. (include dirs(i.e. headers): consumers can't compile without them)
- PRIVATE -> it doesn't. Stays here. (warning flags: my preference, not their problem)

Test: could a consumer compile without this?
- No -> PUBLIC. Yes -> PRIVATE

## Libraries
A library is two artifacts: headers, needed at compile time so your code can reference things; and a compiled binary, needed at link time so those references find bodies.

### Why library + executable
An executable must have exactly one main(). Catch2(test) supplies its own.
So if the lexer lived in the qlang target, tests couldn't link it without including qlang's main() -> two `main()`s not allowed

Solution: all logic in qlang_core (no main), both binaries link it.
- tests (Catch2's main) --> qlang_core(lib .a) <-- qlang (my main)

Consequence: keep main.cpp thin. Anything in it is untestable.

### Commands
    cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
        configure — reads CMakeLists.txt, writes build files. Compiles nothing.
        Only needed when CMakeLists.txt changes.

    cmake --build build -j
        build — runs the generated files, which invoke g++. -j = all cores.

    ctest --test-dir build --output-on-failure
        run tests. Without the flag you get bare pass/fail and no clue why.

    cmake --build build --verbose
        see the actual g++ commands, to check flags really landed.
