cmake_minimum_required(VERSION 3.16)

# Toolchain auto-detection for MSYS2 / Git Bash on Windows
if(WIN32)
    if(CMAKE_GENERATOR STREQUAL "MinGW Makefiles" AND NOT DEFINED CMAKE_MAKE_PROGRAM)
        find_program(MSYS2_MAKE_PROGRAM
            NAMES mingw32-make make
            PATHS "C:/msys64/mingw64/bin" "C:/msys64/ucrt64/bin" "C:/msys64/usr/bin"
            NO_DEFAULT_PATH
        )
        if(MSYS2_MAKE_PROGRAM)
            set(CMAKE_MAKE_PROGRAM "${MSYS2_MAKE_PROGRAM}" CACHE FILEPATH "Detected make" FORCE)
        endif()
    endif()

    if(NOT DEFINED CMAKE_C_COMPILER)
        find_program(MSYS2_C_COMPILER NAMES gcc clang cl
            PATHS "C:/msys64/mingw64/bin" "C:/msys64/ucrt64/bin"
            NO_DEFAULT_PATH
        )
        if(MSYS2_C_COMPILER)
            set(CMAKE_C_COMPILER "${MSYS2_C_COMPILER}" CACHE FILEPATH "Detected C compiler" FORCE)
        endif()
    endif()
endif()

project(HackathonC VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Executable lands in build root, not Debug/Release subfolders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Sources 
set(SOURCES
    src/main.c
    src/input.c
    src/mode.c
    src/control.c
    src/fault.c
    src/state.c
    src/log.c
)

add_executable(ecu_sim ${SOURCES})
target_include_directories(ecu_sim PRIVATE include)

# Compiler warnings 
if(MSVC)
    target_compile_options(ecu_sim PRIVATE /W4 /WX)
else()
    target_compile_options(ecu_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Create logs/ directory next to the executable at build time 
add_custom_command(
    TARGET ecu_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/logs"
    COMMENT "Ensuring logs/ directory exists"
)

# make run  →  runs the C simulator in terminal 
add_custom_target(run
    COMMAND ecu_sim
    DEPENDS ecu_sim
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ecu_sim..."
    USES_TERMINAL
)

# make ui, launches the Python dashboard
find_program(PYTHON_EXECUTABLE NAMES python3 python)

if(PYTHON_EXECUTABLE)
    add_custom_target(ui
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/ui.py
        DEPENDS ecu_sim
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Launching Python ECU Dashboard..."
        USES_TERMINAL
    )
else()
    message(WARNING "Python not found — 'ui' target will not be available.")
endif()