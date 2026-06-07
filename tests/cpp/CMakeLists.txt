# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(KiriViewCppTests LANGUAGES CXX)

enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Multimedia Qml Quick QuickControls2 Test Widgets)
find_package(KF6Archive CONFIG REQUIRED)
find_package(KF6Config CONFIG REQUIRED)
find_package(KF6CoreAddons CONFIG REQUIRED)
find_package(KF6I18n CONFIG REQUIRED)
find_package(KF6KirigamiAddons CONFIG REQUIRED)
find_package(KF6KIO CONFIG REQUIRED)
find_package(KF6JobWidgets CONFIG REQUIRED)
find_package(KF6Service CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LibArchive REQUIRED IMPORTED_TARGET libarchive)
pkg_check_modules(LibHeif REQUIRED IMPORTED_TARGET libheif)
pkg_check_modules(LibJxl REQUIRED IMPORTED_TARGET libjxl)
pkg_check_modules(LibJxlThreads REQUIRED IMPORTED_TARGET libjxl_threads)
pkg_check_modules(LibRaw REQUIRED IMPORTED_TARGET libraw)
pkg_check_modules(LibPng REQUIRED IMPORTED_TARGET libpng)
pkg_check_modules(LibWebP REQUIRED IMPORTED_TARGET libwebp)
pkg_check_modules(LibWebPDemux REQUIRED IMPORTED_TARGET libwebpdemux)
find_program(MSGFMT_EXECUTABLE msgfmt REQUIRED)
find_program(DESKTOP_FILE_VALIDATE_EXECUTABLE desktop-file-validate REQUIRED)

function(kiriview_latest_path output)
    set(latest_path "")
    set(latest_timestamp "")
    foreach(candidate IN LISTS ARGN)
        if(NOT EXISTS "${candidate}")
            continue()
        endif()

        file(TIMESTAMP "${candidate}" candidate_timestamp "%Y%m%d%H%M%S" UTC)
        if(NOT latest_path OR candidate_timestamp STRGREATER latest_timestamp)
            set(latest_path "${candidate}")
            set(latest_timestamp "${candidate_timestamp}")
        endif()
    endforeach()

    set("${output}" "${latest_path}" PARENT_SCOPE)
endfunction()

set(KIRIVIEW_CARGO_TARGET_DIR "${CMAKE_SOURCE_DIR}/../../target/flatpak/debug"
    CACHE PATH "Cargo target debug artifacts directory"
)
set(KIRIVIEW_CARGO_STATICLIB "${KIRIVIEW_CARGO_TARGET_DIR}/libkiriview.a")
file(GLOB KIRIVIEW_CXXQT_INCLUDE_DIR_CANDIDATES
    "${KIRIVIEW_CARGO_TARGET_DIR}/build/kiriview-*/out/cxxqtbuild/include"
)
kiriview_latest_path(KIRIVIEW_CXXQT_INCLUDE_DIR ${KIRIVIEW_CXXQT_INCLUDE_DIR_CANDIDATES})
file(GLOB KIRIVIEW_KCONFIG_INCLUDE_DIR_CANDIDATES
    "${KIRIVIEW_CARGO_TARGET_DIR}/build/kiriview-*/out/kconfig"
)
kiriview_latest_path(KIRIVIEW_KCONFIG_INCLUDE_DIR ${KIRIVIEW_KCONFIG_INCLUDE_DIR_CANDIDATES})
if(NOT EXISTS "${KIRIVIEW_CARGO_STATICLIB}"
    OR NOT KIRIVIEW_CXXQT_INCLUDE_DIR OR NOT EXISTS "${KIRIVIEW_CXXQT_INCLUDE_DIR}"
    OR NOT KIRIVIEW_KCONFIG_INCLUDE_DIR OR NOT EXISTS "${KIRIVIEW_KCONFIG_INCLUDE_DIR}")
    message(FATAL_ERROR "Cargo-owned KiriView app library artifacts are missing. Run the Cargo app library build step first.")
endif()

add_library(KiriViewCargoStatic STATIC IMPORTED GLOBAL)
set_target_properties(KiriViewCargoStatic PROPERTIES
    IMPORTED_LOCATION "${KIRIVIEW_CARGO_STATICLIB}"
)

add_library(kiriview_test_core INTERFACE)
target_include_directories(kiriview_test_core INTERFACE
    ../../src
    "${KIRIVIEW_CXXQT_INCLUDE_DIR}"
    "${KIRIVIEW_KCONFIG_INCLUDE_DIR}"
)
target_compile_definitions(kiriview_test_core INTERFACE
    TRANSLATION_DOMAIN="kiriview"
)
target_link_libraries(kiriview_test_core INTERFACE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,KiriViewCargoStatic>"
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::Archive
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
    KF6::KIOCore
    KF6::KIOGui
    KF6::KIOWidgets
    KF6::JobWidgets
    KF6::Service
    KirigamiAddonsStatefulApp
    PkgConfig::LibArchive
    PkgConfig::LibHeif
    PkgConfig::LibJxl
    PkgConfig::LibJxlThreads
    PkgConfig::LibRaw
    PkgConfig::LibPng
    PkgConfig::LibWebP
    PkgConfig::LibWebPDemux
)

set(KIRIVIEW_TEST_LOCALE_DIR "${CMAKE_CURRENT_BINARY_DIR}/locale")
set(KIRIVIEW_TEST_KO_PO "${CMAKE_SOURCE_DIR}/../fixtures/i18n/ko/kiriview.po")
set(KIRIVIEW_TEST_KO_MO "${KIRIVIEW_TEST_LOCALE_DIR}/ko/LC_MESSAGES/kiriview.mo")
set(KIRIVIEW_TEST_DESKTOP_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/io.github.hnjae.KiriView.desktop"
)

add_custom_command(
    OUTPUT "${KIRIVIEW_TEST_KO_MO}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${KIRIVIEW_TEST_LOCALE_DIR}/ko/LC_MESSAGES"
    COMMAND "${MSGFMT_EXECUTABLE}" --output-file "${KIRIVIEW_TEST_KO_MO}"
        "${KIRIVIEW_TEST_KO_PO}"
    DEPENDS "${KIRIVIEW_TEST_KO_PO}"
    VERBATIM
)

add_custom_command(
    OUTPUT "${KIRIVIEW_TEST_DESKTOP_FILE}"
    COMMAND "${MSGFMT_EXECUTABLE}"
        --desktop
        --locale=ko
        --keyword=Name
        --keyword=GenericName
        --template "${CMAKE_SOURCE_DIR}/../../io.github.hnjae.KiriView.desktop"
        --output-file "${KIRIVIEW_TEST_DESKTOP_FILE}"
        "${KIRIVIEW_TEST_KO_PO}"
    DEPENDS
        "${KIRIVIEW_TEST_KO_PO}"
        "${CMAKE_SOURCE_DIR}/../../io.github.hnjae.KiriView.desktop"
    VERBATIM
)

add_custom_target(kiriview_test_i18n ALL
    DEPENDS
        "${KIRIVIEW_TEST_KO_MO}"
        "${KIRIVIEW_TEST_DESKTOP_FILE}"
)

function(add_kiriview_cpp_test target source)
    add_executable(${target} ${source})
    target_compile_definitions(${target} PRIVATE
        KIRIVIEW_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
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

foreach(kdirnotify_test IN ITEMS test_imagedocumentpagecandidatedirectoryentry test_imagedocumentpagecandidatestore)
    if(TARGET ${kdirnotify_test})
        set_tests_properties(${kdirnotify_test} PROPERTIES
            RESOURCE_LOCK kdirnotify
        )
    endif()
endforeach()

if(TARGET test_localization)
    add_dependencies(test_localization kiriview_test_i18n)
    target_compile_definitions(test_localization PRIVATE
        KIRIVIEW_TEST_DESKTOP_FILE="${KIRIVIEW_TEST_DESKTOP_FILE}"
        KIRIVIEW_TEST_LOCALE_DIR="${KIRIVIEW_TEST_LOCALE_DIR}"
    )
    set_tests_properties(test_localization PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_toastnotification)
    set_tests_properties(test_toastnotification PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_imageactions)
    set_tests_properties(test_imageactions PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_menuaccesskeyrouter)
    set_tests_properties(test_menuaccesskeyrouter PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_menuactionitem)
    set_tests_properties(test_menuactionitem PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_mainwindowtoolbar)
    set_tests_properties(test_mainwindowtoolbar PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2.075"
    )
endif()

if(TARGET test_mainwindowvideointegration)
    set_tests_properties(test_mainwindowvideointegration PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_imagezoomcontrols)
    set_tests_properties(test_imagezoomcontrols PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_thumbnailpanel)
    set_tests_properties(test_thumbnailpanel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_toolbarapplicationmenu)
    set_tests_properties(test_toolbarapplicationmenu PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_imageshortcuts)
    set_tests_properties(test_imageshortcuts PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_imageviewport)
    set_tests_properties(test_imageviewport PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_displayimagepage)
    set_tests_properties(test_displayimagepage PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_videodocumentruntimezoom)
    set_tests_properties(test_videodocumentruntimezoom PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_videooutputruntime)
    set_tests_properties(test_videooutputruntime PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_shortcuthelpdialog)
    set_tests_properties(test_shortcuthelpdialog PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

if(TARGET test_shortcuthelpmodel)
    set_tests_properties(test_shortcuthelpmodel PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

add_test(
    NAME test_desktop_file_validate
    COMMAND "${DESKTOP_FILE_VALIDATE_EXECUTABLE}" "${KIRIVIEW_TEST_DESKTOP_FILE}"
)
