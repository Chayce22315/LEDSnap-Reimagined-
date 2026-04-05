cmake_minimum_required(VERSION 3.15)
project(libledsnap C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(ledsnap STATIC
    src/ls_compositor.c
    src/ls_tools.c
    src/ls_text.c
    src/ls_animation.c
    src/ls_lsnap.c
    src/ls_frame_limiter.c
)

target_include_directories(ledsnap PUBLIC include)

if(MSVC)
    target_compile_options(ledsnap PRIVATE /W4)
else()
    target_compile_options(ledsnap PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(ledsnap PRIVATE m)
endif()

# --- test executable ---
add_executable(test_compositor tests/test_compositor.c)
target_link_libraries(test_compositor PRIVATE ledsnap)
if(NOT MSVC)
    target_link_libraries(test_compositor PRIVATE m)
endif()
