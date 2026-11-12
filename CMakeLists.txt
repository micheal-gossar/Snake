cmake_minimum_required(VERSION 3.25)
project(Snake VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

# SDL 3.4.20 and Dear ImGui 1.92.9b, pinned to immutable commits.
FetchContent_Declare(SDL3
    URL https://codeload.github.com/libsdl-org/SDL/tar.gz/f00dc919ca35b9756316e86c4777392de442f9b2
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/f1cc2ae15e53a861a874c3034aae6798fde194ab
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable(SDL3 imgui)

add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp"
)
target_include_directories(imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

file(GLOB_RECURSE SNAKE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)
file(GLOB_RECURSE SNAKE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
    "${CMAKE_CURRENT_SOURCE_DIR}/headers/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/headers/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/headers/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/headers/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/headers/*.inl"
)

if(NOT SNAKE_SOURCES)
    message(STATUS "No application sources in src. Add an entry point to enable Run and F5.")
    add_custom_target(snake ALL
        COMMAND "${CMAKE_COMMAND}" -E echo
            "No application sources in src. Add an entry point to enable Run and F5."
        SOURCES ${SNAKE_HEADERS}
        VERBATIM
    )
    return()
endif()

add_executable(snake ${SNAKE_SOURCES} ${SNAKE_HEADERS})
target_include_directories(snake PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/headers"
)
target_link_libraries(snake PRIVATE imgui)
if(MSVC)
    # MSVC exposes draft C++26 through /std:c++latest, which CMake cannot yet select as CXX_STANDARD.
    target_compile_options(snake PRIVATE /std:c++latest /W4 /permissive-)
else()
    target_compile_options(snake PRIVATE -Wall -Wextra -Wpedantic)
endif()
