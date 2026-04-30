// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_build::{CppFile, CxxQtBuilder, QmlModule};
use std::{
    collections::BTreeSet,
    env,
    path::{Path, PathBuf},
};

fn main() {
    let kio_include_dirs = kio_include_dirs();
    link_kio();

    let mut builder = CxxQtBuilder::new_qml_module(
        QmlModule::new("io.github.hnjae.kiriview").qml_file("src/qml/Main.qml"),
    )
    .cpp_file(CppFile::from("src/kiriimageview.h"))
    .cpp_file("src/apngdecoder.cpp")
    .cpp_file("src/avifcompat.cpp")
    .cpp_file("src/kiriimageview.cpp")
    .qt_module("Quick")
    .qt_module("Network")
    .qt_module("DBus");

    unsafe {
        builder = builder.cc_builder(move |cc| {
            cc.flag_if_supported("-Wno-attributes");
            for dir in &kio_include_dirs {
                cc.include(dir);
            }
        });
    }

    builder.build();
}

fn link_kio() {
    println!("cargo::rerun-if-env-changed=NIX_LDFLAGS");

    for dir in kio_library_dirs() {
        println!("cargo::rustc-link-search=native={}", dir.display());
    }

    println!("cargo::rustc-link-lib=KF6KIOCore");
    println!("cargo::rustc-link-lib=KF6CoreAddons");
}

fn kio_include_dirs() -> Vec<PathBuf> {
    println!("cargo::rerun-if-env-changed=NIX_CFLAGS_COMPILE");

    let mut dirs = BTreeSet::new();
    add_kf6_include_dirs(&mut dirs, Path::new("/usr/include"));
    add_qt_mkspec_include_dirs(&mut dirs, Path::new("/usr/include"));

    for path in flag_paths("NIX_CFLAGS_COMPILE", "-isystem") {
        add_kf6_include_dirs(&mut dirs, &path);
        add_qt_mkspec_include_dirs(&mut dirs, &path);
    }
    for path in flag_paths("NIX_CFLAGS_COMPILE", "-I") {
        add_kf6_include_dirs(&mut dirs, &path);
        add_qt_mkspec_include_dirs(&mut dirs, &path);
    }

    dirs.into_iter().collect()
}

fn add_kf6_include_dirs(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    for suffix in ["KF6/KIOCore", "KF6/KIO", "KF6/KCoreAddons"] {
        let dir = include_root.join(suffix);
        if dir.exists() {
            dirs.insert(dir);
        }
    }
}

fn add_qt_mkspec_include_dirs(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    let Some(qt_root) = include_root.parent() else {
        return;
    };

    let dir = qt_root.join("mkspecs/linux-g++");
    if dir.exists() {
        dirs.insert(dir);
    }
}

fn kio_library_dirs() -> Vec<PathBuf> {
    let mut dirs = BTreeSet::new();
    for dir in ["/usr/lib/x86_64-linux-gnu", "/usr/lib"] {
        let path = PathBuf::from(dir);
        if path.exists() {
            dirs.insert(path);
        }
    }

    for path in flag_paths("NIX_LDFLAGS", "-L") {
        if contains_kio_library(&path) || contains_core_addons_library(&path) {
            dirs.insert(path);
        }
    }

    dirs.into_iter().collect()
}

fn contains_kio_library(dir: &Path) -> bool {
    dir.join("libKF6KIOCore.so").exists()
}

fn contains_core_addons_library(dir: &Path) -> bool {
    dir.join("libKF6CoreAddons.so").exists()
}

fn flag_paths(env_var: &str, flag: &str) -> Vec<PathBuf> {
    let value = env::var(env_var).unwrap_or_default();
    let mut paths = Vec::new();
    let mut tokens = value.split_whitespace();

    while let Some(token) = tokens.next() {
        if token == flag {
            if let Some(path) = tokens.next() {
                paths.push(PathBuf::from(path));
            }
        } else if let Some(path) = token.strip_prefix(flag)
            && !path.is_empty()
        {
            paths.push(PathBuf::from(path));
        }
    }

    paths
}
