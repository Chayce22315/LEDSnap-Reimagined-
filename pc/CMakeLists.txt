cmake_minimum_required(VERSION 3.15)
project(ledsnap_receiver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Core engine library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_BINARY_DIR}/core)

# Find SDL2 - supports both system-installed and vcpkg
find_package(SDL2 REQUIRED)

add_executable(ledsnap_receiver
    src/main.c
    src/renderer.c
    src/receiver.c
)

target_include_directories(ledsnap_receiver PRIVATE src)

# SDL2 target varies between system and vcpkg installs
if(TARGET SDL2::SDL2)
    target_link_libraries(ledsnap_receiver PRIVATE ledsnap SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(ledsnap_receiver PRIVATE SDL2::SDL2main)
    endif()
else()
    target_include_directories(ledsnap_receiver PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ledsnap_receiver PRIVATE ledsnap ${SDL2_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(ledsnap_receiver PRIVATE /W4)
    # Copy SDL2.dll next to the .exe on Windows
    if(TARGET SDL2::SDL2)
        add_custom_command(TARGET ledsnap_receiver POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL2::SDL2>
                $<TARGET_FILE_DIR:ledsnap_receiver>
        )
    endif()
else()
    target_compile_options(ledsnap_receiver PRIVATE -Wall -Wextra)
    target_link_libraries(ledsnap_receiver PRIVATE m)
endif()

if(WIN32)
    set_target_properties(ledsnap_receiver PROPERTIES
        OUTPUT_NAME "LEDSnap_Receiver"
        WIN32_EXECUTABLE FALSE
    )
endif()
