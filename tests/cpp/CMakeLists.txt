# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(KiriViewCppTests LANGUAGES CXX)

enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg Test Widgets)
find_package(KF6Archive CONFIG REQUIRED)
find_package(KF6CoreAddons CONFIG REQUIRED)
find_package(KF6KIO CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LibArchive REQUIRED IMPORTED_TARGET libarchive)
pkg_check_modules(LibHeif REQUIRED IMPORTED_TARGET libheif)

set(KIRIVIEW_CARGO_TARGET_DIR "${CMAKE_SOURCE_DIR}/../../target/flatpak/debug")
file(GLOB KIRIVIEW_RUST_STATICLIBS
    "${KIRIVIEW_CARGO_TARGET_DIR}/deps/libkiriview-*.a"
)
list(SORT KIRIVIEW_RUST_STATICLIBS)
list(POP_BACK KIRIVIEW_RUST_STATICLIBS KIRIVIEW_RUST_STATICLIB)
file(GLOB KIRIVIEW_CXXQT_INCLUDE_DIRS
    "${KIRIVIEW_CARGO_TARGET_DIR}/build/kiriview-*/out/cxxqtbuild/include"
)
list(SORT KIRIVIEW_CXXQT_INCLUDE_DIRS)
list(POP_BACK KIRIVIEW_CXXQT_INCLUDE_DIRS KIRIVIEW_CXXQT_INCLUDE_DIR)
if(NOT KIRIVIEW_CXXQT_INCLUDE_DIR OR NOT EXISTS "${KIRIVIEW_RUST_STATICLIB}")
    message(FATAL_ERROR "Rust bridge artifacts are missing. Run the Cargo test/build step first.")
endif()

file(STRINGS "../../src/cpp_core_sources.txt" KIRIVIEW_CORE_SOURCES
    REGEX "^[A-Za-z0-9_./-]+\\.cpp$"
)
foreach(source IN LISTS KIRIVIEW_CORE_SOURCES)
    list(APPEND KIRIVIEW_CORE_SOURCE_PATHS "../../${source}")
endforeach()

add_library(kiriview_test_core STATIC
    ${KIRIVIEW_CORE_SOURCE_PATHS}
)
target_include_directories(kiriview_test_core PUBLIC ../../src "${KIRIVIEW_CXXQT_INCLUDE_DIR}")
target_link_libraries(kiriview_test_core PUBLIC
    "${KIRIVIEW_RUST_STATICLIB}"
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Widgets
    KF6::Archive
    KF6::CoreAddons
    KF6::KIOCore
    PkgConfig::LibArchive
    PkgConfig::LibHeif
)

function(add_kiriview_cpp_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE
        kiriview_test_core
        Qt6::Test
    )
    add_test(NAME ${target} COMMAND ${target})
endfunction()

file(GLOB KIRIVIEW_CPP_TEST_SOURCES CONFIGURE_DEPENDS "test_*.cpp")
list(SORT KIRIVIEW_CPP_TEST_SOURCES)
foreach(test_source IN LISTS KIRIVIEW_CPP_TEST_SOURCES)
    get_filename_component(test_target "${test_source}" NAME_WE)
    add_kiriview_cpp_test("${test_target}" "${test_source}")
endforeach()
