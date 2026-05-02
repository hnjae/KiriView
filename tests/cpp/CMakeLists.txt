# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(KiriViewCppTests LANGUAGES CXX)

enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)
find_package(KF6CoreAddons CONFIG REQUIRED)
find_package(KF6KIO CONFIG REQUIRED)

add_library(kiriview_test_core STATIC
    ../../src/imagebytecost.cpp
    ../../src/imagecontainer.cpp
    ../../src/imageformatregistry.cpp
    ../../src/imagenavigationmodel.cpp
    ../../src/imageurl.cpp
    ../../src/imagezoomstate.cpp
    ../../src/predecodecache.cpp
)
target_include_directories(kiriview_test_core PUBLIC ../../src)
target_link_libraries(kiriview_test_core PUBLIC
    Qt6::Core
    Qt6::Gui
    KF6::CoreAddons
    KF6::KIOCore
)

function(add_kiriview_cpp_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE
        kiriview_test_core
        Qt6::Test
    )
    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_kiriview_cpp_test(test_imagezoomstate test_imagezoomstate.cpp)
add_kiriview_cpp_test(test_imagecontainer test_imagecontainer.cpp)
add_kiriview_cpp_test(test_imagenavigationmodel test_imagenavigationmodel.cpp)
add_kiriview_cpp_test(test_imageurl test_imageurl.cpp)
add_kiriview_cpp_test(test_predecodecache test_predecodecache.cpp)
