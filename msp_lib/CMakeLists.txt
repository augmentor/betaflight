cmake_minimum_required(VERSION 3.15)
project(msp_lib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Platform flags ────────────────────────────────────────
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX _USE_MATH_DEFINES)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Library target ────────────────────────────────────────
add_library(msp_lib
    src/serial_port.cpp
    src/msp_codec.cpp
    src/msp_client.cpp
    src/msp_commands.cpp
)

target_include_directories(msp_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(msp_lib PUBLIC cxx_std_17)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(msp_lib PRIVATE setupapi)
elseif(APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CF_LIBRARY CoreFoundation)
    target_link_libraries(msp_lib PRIVATE ${IOKIT_LIBRARY} ${CF_LIBRARY})
endif()

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(msp_lib PRIVATE Threads::Threads)
endif()

# ── Examples ─────────────────────────────────────────────
option(MSP_BUILD_EXAMPLES "Build example programs" ON)
if(MSP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── Tests ────────────────────────────────────────────────
option(MSP_BUILD_TESTS "Build unit tests" ON)
if(MSP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────
install(TARGETS msp_lib
    EXPORT msp_libTargets
    LIBRARY  DESTINATION lib
    ARCHIVE  DESTINATION lib
    RUNTIME  DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT msp_libTargets
    FILE msp_libTargets.cmake
    NAMESPACE msp::
    DESTINATION lib/cmake/msp_lib
)
