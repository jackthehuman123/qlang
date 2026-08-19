# Configure file for CMake
cmake_minimum_required(VERSION 3.20)    # Minimum version to run
project(qlang LANGUAGES CXX)            # project(name language)

set(CMAKE_CXX_STANDARD 20)              # language setting 
                                        # CMAKE_CXX_STANDARD 20 is -std=c++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # Enforcement

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # generates `build/compile_commands.json` with flags
                                        # g++ -std=c++20 -Iinclude -Wall -Wextra -c src/main.cpp -o main.o
                                        #     └──────────────────────────────┘        └────────┘   └──────┘
                                        #               flags                          the input    output

# --- Sanitizers: must come BEFORE any target is defined ---
option(ENABLE_SANITIZERS "Build with AddressSanitizer + UBSan" OFF)     # default OFF in case of release build
if (ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    # g++ inserts checks into my machine code; those checks call functions that
    # don't exist yet — same as declaring without defining
    add_link_options(-fsanitize=address,undefined)      
    # hands the libraries to the linker so those calls find real bodies
endif()

# --- Core library: all logic, no main() ---
#     Every new .cpp in src/ goes in this list. ---
add_library(qlang_core
    src/lexer.cpp
)
# lexer.cpp         source file      text you wrote
#     ↓ g++ 
# lexer.o           object file      machine code, one per .cpp
#     ↓ ar
# libqlang_core.a   static library   bundle of .o files

# when compiling qlang_core, search for headers in include/
# PUBLIC = anything that links qlang_core searches there too
target_include_directories(qlang_core PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include)

# warnings on, this target only
# Best practice: consumers should NOT inherit warnings flags
target_compile_options(qlang_core PRIVATE 
    -Wall 
    -Wextra 
    -Wpedantic
)

# --- CLI: just main(), links the library ---
add_executable(qlang src/main.cpp)          

# pulls in qlang_core's compiled code + its PUBLIC include path
target_link_libraries(qlang PRIVATE 
    qlang_core
)

# Best practice
target_compile_options(qlang PRIVATE 
    -Wall       # Flags
    -Wextra 
    -Wpedantic
)

# target must be 'add'-ed first, ie: 
# add_library(qlang_core
#     src/lexer.cpp
# )
# Then configured

# --- Catch2, downloaded automatically ---
include(FetchContent)   # include(module) -- Loads a CMake module
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)  # clone and configure

enable_testing()    # Turn on CTest, CMake's test runner

add_executable(tests tests/test_lexer.cpp)

target_link_libraries(tests PRIVATE 
    qlang_core              # These are 2 separate libraries
    Catch2::Catch2WithMain
)

# tell CMake to also search Catch2's extras/ folder for .cmake modules
# ${catch2_SOURCE_DIR} was set by FetchContent_MakeAvailable — note the lowercase c
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# CMake's include, not C++'s: loads and runs extras/Catch.cmake,
# which is what defines catch_discover_tests below
include(Catch)

# scans the built test binary and registers each TEST_CASE with CTest
# individually, so a failure names the specific case
catch_discover_tests(tests)

# cmake -B build -DCMAKE_BUILD_TYPE=Debug 
# this creates a "build" folder that contains all the files built by cmake