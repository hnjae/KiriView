// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_build::{CppFile, CxxQtBuilder, QmlModule};
use std::{
    collections::BTreeSet,
    env, fs,
    path::{Component, Path, PathBuf},
    process::Command,
};

type IncludeDirCollector = fn(&mut BTreeSet<PathBuf>, &Path);

struct IncludeSearch {
    collectors: &'static [IncludeDirCollector],
    pkg_config_packages: &'static [&'static str],
}

const CPP_CORE_SOURCES_FILE: &str = "src/cpp_core_sources.txt";
const CXX_QT_HEADER_SOURCES_FILE: &str = "src/cpp_cxxqt_header_sources.txt";
const CXX_QT_CPP_SOURCES_FILE: &str = "src/cpp_cxxqt_sources.txt";
const RUST_POLICY_SOURCES_FILE: &str = "src/rust_policy_sources.txt";
const RUST_BRIDGE_SOURCES_FILE: &str = "src/rust_bridge_sources.txt";
const KCONFIG_SCHEMA_FILE: &str = "src/kiriviewstate.kcfg";
const KCONFIG_COMPILER_FILE: &str = "src/kiriviewstate.kcfgc";
const QML_SOURCE_DIR: &str = "src/qml";
const DEFAULT_INCLUDE_ROOTS: &[&str] = &["/app/include", "/usr/include"];
const DEFAULT_LIBRARY_DIRS: &[&str] = &["/app/lib", "/usr/lib/x86_64-linux-gnu", "/usr/lib"];
const QT_MODULES: &[&str] = &[
    "Gui",
    "Multimedia",
    "Qml",
    "Quick",
    "QuickControls2",
    "Network",
    "DBus",
    "Widgets",
];
const NATIVE_LIBRARIES: &[NativeLibrary] = &[
    NativeLibrary {
        link_name: "KF6Archive",
        file_name: "libKF6Archive.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6KIOCore",
        file_name: "libKF6KIOCore.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6KIOGui",
        file_name: "libKF6KIOGui.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6KIOWidgets",
        file_name: "libKF6KIOWidgets.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6JobWidgets",
        file_name: "libKF6JobWidgets.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6Service",
        file_name: "libKF6Service.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6CoreAddons",
        file_name: "libKF6CoreAddons.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6ConfigCore",
        file_name: "libKF6ConfigCore.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6ConfigGui",
        file_name: "libKF6ConfigGui.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6I18n",
        file_name: "libKF6I18n.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KF6I18nQml",
        file_name: "libKF6I18nQml.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "KirigamiAddonsStatefulApp",
        file_name: "libKirigamiAddonsStatefulApp.so",
        pkg_config_package: None,
    },
    NativeLibrary {
        link_name: "archive",
        file_name: "libarchive.so",
        pkg_config_package: Some("libarchive"),
    },
    NativeLibrary {
        link_name: "heif",
        file_name: "libheif.so",
        pkg_config_package: Some("libheif"),
    },
    NativeLibrary {
        link_name: "jxl_threads",
        file_name: "libjxl_threads.so",
        pkg_config_package: Some("libjxl_threads"),
    },
    NativeLibrary {
        link_name: "jxl",
        file_name: "libjxl.so",
        pkg_config_package: Some("libjxl"),
    },
    NativeLibrary {
        link_name: "raw",
        file_name: "libraw.so",
        pkg_config_package: Some("libraw"),
    },
    NativeLibrary {
        link_name: "webpdemux",
        file_name: "libwebpdemux.so",
        pkg_config_package: Some("libwebpdemux"),
    },
    NativeLibrary {
        link_name: "webp",
        file_name: "libwebp.so",
        pkg_config_package: Some("libwebp"),
    },
];
const NO_PKG_CONFIG_PACKAGES: &[&str] = &[];
const KIO_INCLUDE_COLLECTORS: &[IncludeDirCollector] =
    &[add_kf6_include_dirs, add_qt_mkspec_include_dirs];
const QT_QML_INTEGRATION_INCLUDE_COLLECTORS: &[IncludeDirCollector] =
    &[add_qt_qml_integration_include_dirs];
const LIBARCHIVE_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libarchive_include_dir];
const LIBHEIF_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libheif_include_dir];
const LIBJXL_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libjxl_include_dir];
const LIBRAW_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libraw_include_dir];
const LIBWEBP_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libwebp_include_dir];
const NATIVE_INCLUDE_SEARCHES: &[IncludeSearch] = &[
    IncludeSearch {
        collectors: KIO_INCLUDE_COLLECTORS,
        pkg_config_packages: NO_PKG_CONFIG_PACKAGES,
    },
    IncludeSearch {
        collectors: LIBARCHIVE_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libarchive"],
    },
    IncludeSearch {
        collectors: LIBHEIF_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libheif"],
    },
    IncludeSearch {
        collectors: LIBJXL_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libjxl", "libjxl_threads"],
    },
    IncludeSearch {
        collectors: LIBRAW_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libraw"],
    },
    IncludeSearch {
        collectors: LIBWEBP_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libwebp", "libwebpdemux"],
    },
    IncludeSearch {
        collectors: QT_QML_INTEGRATION_INCLUDE_COLLECTORS,
        pkg_config_packages: &["Qt6QmlIntegration"],
    },
];

struct NativeLibrary {
    link_name: &'static str,
    file_name: &'static str,
    pkg_config_package: Option<&'static str>,
}

struct GeneratedState {
    include_dir: PathBuf,
    source: PathBuf,
}

fn main() {
    let native_include_dirs = native_include_dirs();
    let cxx_qt_header_sources = source_manifest(CXX_QT_HEADER_SOURCES_FILE);
    let rust_policy_sources = source_manifest(RUST_POLICY_SOURCES_FILE);
    let rust_bridge_sources = source_manifest(RUST_BRIDGE_SOURCES_FILE);
    let cxx_qt_cpp_sources = source_manifest(CXX_QT_CPP_SOURCES_FILE);
    let cpp_core_sources = source_manifest(CPP_CORE_SOURCES_FILE);
    validate_source_manifest_extensions(&cxx_qt_header_sources, CXX_QT_HEADER_SOURCES_FILE, "h");
    validate_source_manifest_extensions(&rust_policy_sources, RUST_POLICY_SOURCES_FILE, "rs");
    validate_source_manifest_extensions(&rust_bridge_sources, RUST_BRIDGE_SOURCES_FILE, "rs");
    validate_source_manifest_extensions(&cxx_qt_cpp_sources, CXX_QT_CPP_SOURCES_FILE, "cpp");
    validate_source_manifest_extensions(&cpp_core_sources, CPP_CORE_SOURCES_FILE, "cpp");
    validate_cpp_source_ownership(&cpp_core_sources, &cxx_qt_cpp_sources);
    validate_rust_policy_source_ownership(&rust_policy_sources, &rust_bridge_sources);
    let generated_state = generate_kconfig_state();
    let generated_state_source = generated_state.source.to_string_lossy().into_owned();
    let qml_module = qml_module();
    link_native_libraries();

    let mut builder = CxxQtBuilder::new_qml_module(qml_module)
        // Do not export the whole crate root as C++ headers. That makes Cargo watch
        // build artifacts such as build-dir/, including symlinks into /run.
        .crate_include_root(None);

    builder = add_cpp_file_strings(builder, cxx_qt_header_sources);
    builder = builder.cpp_file(CppFile::from(generated_state_source.as_str()));
    builder = add_rust_file_strings(builder, rust_bridge_sources);
    builder = add_cpp_file_strings(builder, cxx_qt_cpp_sources);
    builder = add_cpp_file_strings(builder, cpp_core_sources);
    builder = add_qt_modules(builder, QT_MODULES);

    unsafe {
        builder = builder.cc_builder(move |cc| {
            cc.flag_if_supported("-Wno-attributes");
            cc.define("TRANSLATION_DOMAIN", "\"kiriview\"");
            cc.include("src");
            cc.include(&generated_state.include_dir);
            for dir in &native_include_dirs {
                cc.include(dir);
            }
        });
    }

    builder.build();
}

fn add_cpp_file_strings(mut builder: CxxQtBuilder, files: Vec<String>) -> CxxQtBuilder {
    for file in files {
        builder = builder.cpp_file(CppFile::from(file.as_str()));
    }
    builder
}

fn add_rust_file_strings(mut builder: CxxQtBuilder, files: Vec<String>) -> CxxQtBuilder {
    for file in files {
        builder = builder.file(file.as_str());
    }
    builder
}

fn add_qt_modules(mut builder: CxxQtBuilder, modules: &[&str]) -> CxxQtBuilder {
    for module in modules {
        builder = builder.qt_module(module);
    }
    builder
}

fn qml_module() -> QmlModule {
    let mut module = QmlModule::new("org.hnjae.kiriview")
        .depend("QtMultimedia")
        .depend("QtQuick")
        .depend("org.kde.kirigamiaddons.formcard")
        .depend("org.kde.kirigamiaddons.statefulapp");
    for qml_file in qml_files() {
        module = module.qml_file(qml_file);
    }
    module
}

fn generate_kconfig_state() -> GeneratedState {
    println!("cargo::rerun-if-changed={KCONFIG_SCHEMA_FILE}");
    println!("cargo::rerun-if-changed={KCONFIG_COMPILER_FILE}");

    let out_dir = PathBuf::from(env::var_os("OUT_DIR").expect("OUT_DIR must be set"));
    let include_dir = out_dir.join("kconfig");
    fs::create_dir_all(&include_dir).expect("failed to create KConfigXT output directory");

    let status = Command::new(kconfig_compiler())
        .args(["-d"])
        .arg(&include_dir)
        .arg(KCONFIG_SCHEMA_FILE)
        .arg(KCONFIG_COMPILER_FILE)
        .status()
        .expect("failed to run kconfig_compiler_kf6; make sure KConfig is installed");

    if !status.success() {
        panic!("kconfig_compiler_kf6 failed for {KCONFIG_SCHEMA_FILE}");
    }

    GeneratedState {
        include_dir: include_dir.clone(),
        source: include_dir.join("kiriviewstate.cpp"),
    }
}

fn qml_files() -> Vec<String> {
    println!("cargo::rerun-if-changed={QML_SOURCE_DIR}");

    let mut files = Vec::new();
    for entry in fs::read_dir(QML_SOURCE_DIR).expect("failed to read QML source directory") {
        let path = entry
            .expect("failed to read QML source directory entry")
            .path();
        if path.extension().is_some_and(|extension| extension == "qml") {
            println!("cargo::rerun-if-changed={}", path.display());
            files.push(path.to_string_lossy().into_owned());
        }
    }
    files.sort();
    files
}

fn source_manifest(path: &str) -> Vec<String> {
    println!("cargo::rerun-if-changed={path}");
    let mut seen = BTreeSet::new();
    fs::read_to_string(path)
        .unwrap_or_else(|error| panic!("failed to read source manifest {path}: {error}"))
        .lines()
        .map(str::trim)
        .filter(|line| !line.is_empty() && !line.starts_with('#'))
        .map(|line| {
            validate_source_manifest_path(path, line);
            if !seen.insert(line.to_owned()) {
                panic!("duplicate source manifest entry {line} in {path}");
            }
            line.to_owned()
        })
        .collect()
}

fn validate_source_manifest_path(manifest: &str, source: &str) {
    let path = Path::new(source);
    if path.is_absolute() {
        panic!("source manifest entry {source} in {manifest} must be relative");
    }

    if path.components().any(|component| {
        matches!(
            component,
            Component::ParentDir | Component::RootDir | Component::Prefix(_)
        )
    }) {
        panic!("source manifest entry {source} in {manifest} must stay inside the repository");
    }

    if !path.starts_with("src") {
        panic!("source manifest entry {source} in {manifest} must live under src/");
    }

    if !path.exists() {
        panic!("source manifest entry {source} in {manifest} does not exist");
    }
}

fn validate_source_manifest_extensions(sources: &[String], manifest: &str, extension: &str) {
    for source in sources {
        if Path::new(source)
            .extension()
            .and_then(|value| value.to_str())
            != Some(extension)
        {
            panic!("source manifest entry {source} in {manifest} must be a .{extension} file");
        }
    }
}

fn validate_cpp_source_ownership(cpp_core_sources: &[String], cxx_qt_cpp_sources: &[String]) {
    let mut manifest_sources = BTreeSet::new();
    for source in cpp_core_sources.iter().chain(cxx_qt_cpp_sources) {
        if !manifest_sources.insert(source.clone()) {
            panic!("C++ source {source} is listed in more than one source manifest");
        }
    }

    let discovered_sources = discovered_cpp_sources(Path::new("src"));
    for source in &discovered_sources {
        if !manifest_sources.contains(source) {
            panic!("C++ source {source} is not listed in a source manifest");
        }
    }
}

fn validate_rust_policy_source_ownership(
    rust_policy_sources: &[String],
    rust_bridge_sources: &[String],
) {
    let policy_source_set: BTreeSet<String> = rust_policy_sources.iter().cloned().collect();

    for source in rust_policy_sources {
        if !Path::new(source).starts_with("src/policy") {
            panic!("Rust policy source {source} must live under src/policy/");
        }
    }

    for source in rust_bridge_sources {
        if !policy_source_set.contains(source) {
            panic!(
                "Rust bridge source {source} is not listed in the Rust policy source manifest {RUST_POLICY_SOURCES_FILE}"
            );
        }
    }

    let discovered_sources = discovered_rs_sources(Path::new("src/policy"));
    for source in &discovered_sources {
        if !policy_source_set.contains(source) {
            panic!("Rust policy source {source} is not listed in {RUST_POLICY_SOURCES_FILE}");
        }
    }
}

fn discovered_cpp_sources(root: &Path) -> BTreeSet<String> {
    let mut sources = BTreeSet::new();
    collect_cpp_sources(root, &mut sources);
    sources
}

fn discovered_rs_sources(root: &Path) -> BTreeSet<String> {
    let mut sources = BTreeSet::new();
    collect_sources_with_extension(root, "rs", &mut sources);
    sources
}

fn collect_cpp_sources(dir: &Path, sources: &mut BTreeSet<String>) {
    collect_sources_with_extension(dir, "cpp", sources);
}

fn collect_sources_with_extension(dir: &Path, extension: &str, sources: &mut BTreeSet<String>) {
    for entry in fs::read_dir(dir).unwrap_or_else(|error| {
        panic!("failed to read source directory {}: {error}", dir.display())
    }) {
        let path = entry
            .unwrap_or_else(|error| {
                panic!(
                    "failed to read source directory entry under {}: {error}",
                    dir.display()
                )
            })
            .path();
        if path.is_dir() {
            collect_sources_with_extension(&path, extension, sources);
        } else if path.extension().and_then(|value| value.to_str()) == Some(extension) {
            sources.insert(path.to_string_lossy().replace('\\', "/"));
        }
    }
}

fn link_native_libraries() {
    println!("cargo::rerun-if-env-changed=NIX_LDFLAGS");
    println!("cargo::rerun-if-env-changed=PKG_CONFIG_PATH");

    for dir in native_library_dirs() {
        println!("cargo::rustc-link-search=native={}", dir.display());
    }

    for library in NATIVE_LIBRARIES {
        println!("cargo::rustc-link-lib={}", library.link_name);
    }
}

fn native_include_dirs() -> Vec<PathBuf> {
    let mut dirs = BTreeSet::new();
    for search in NATIVE_INCLUDE_SEARCHES {
        dirs.extend(include_dirs(search.collectors, search.pkg_config_packages));
    }
    dirs.into_iter().collect()
}

fn include_dirs(collectors: &[IncludeDirCollector], pkg_config_packages: &[&str]) -> Vec<PathBuf> {
    println!("cargo::rerun-if-env-changed=NIX_CFLAGS_COMPILE");
    if !pkg_config_packages.is_empty() {
        println!("cargo::rerun-if-env-changed=PKG_CONFIG_PATH");
    }

    let mut dirs = BTreeSet::new();
    for root in include_candidate_roots(pkg_config_packages) {
        for collector in collectors {
            collector(&mut dirs, &root);
        }
    }

    dirs.into_iter().collect()
}

fn include_candidate_roots(pkg_config_packages: &[&str]) -> Vec<PathBuf> {
    let mut roots: Vec<PathBuf> = DEFAULT_INCLUDE_ROOTS.iter().map(PathBuf::from).collect();
    roots.extend(flag_paths("NIX_CFLAGS_COMPILE", "-isystem"));
    roots.extend(flag_paths("NIX_CFLAGS_COMPILE", "-I"));
    for package in pkg_config_packages {
        roots.extend(pkg_config_include_dirs(package));
    }
    roots
}

fn add_libarchive_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("archive.h").exists() && include_root.join("archive_entry.h").exists() {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_libheif_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("libheif").join("heif.h").exists() {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_libjxl_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("jxl").join("decode.h").exists()
        && include_root
            .join("jxl")
            .join("thread_parallel_runner.h")
            .exists()
    {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_libraw_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("libraw").join("libraw.h").exists() {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_libwebp_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("webp").join("decode.h").exists()
        && include_root.join("webp").join("demux.h").exists()
    {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_qt_qml_integration_include_dirs(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    for candidate in [
        include_root.to_path_buf(),
        include_root.join("QtQmlIntegration"),
    ] {
        if candidate.join("QtQmlIntegration").exists()
            && candidate.join("qqmlintegration.h").exists()
        {
            dirs.insert(candidate);
        }
    }
}

fn pkg_config_include_dirs(package: &str) -> Vec<PathBuf> {
    pkg_config_paths(package, "--cflags", &["-I", "-isystem"])
}

fn pkg_config_paths(package: &str, pkg_config_arg: &str, flags: &[&str]) -> Vec<PathBuf> {
    let output = Command::new("pkg-config")
        .args([pkg_config_arg, package])
        .output();
    let Ok(output) = output else {
        return Vec::new();
    };
    if !output.status.success() {
        return Vec::new();
    }

    let paths = String::from_utf8_lossy(&output.stdout);
    flag_paths_from_tokens(paths.split_whitespace(), flags)
}

fn add_kf6_include_dirs(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    for suffix in [
        "KF6/KArchive",
        "KF6/KIOCore",
        "KF6/KIOGui",
        "KF6/KIOWidgets",
        "KF6/KIO",
        "KF6/KJobWidgets",
        "KF6/KService",
        "KF6/KCoreAddons",
        "KF6/KConfig",
        "KF6/KConfigCore",
        "KF6/KConfigGui",
        "KF6/KI18n",
        "KF6/KI18nQml",
        "KirigamiAddonsStatefulApp",
    ] {
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

fn native_library_dirs() -> Vec<PathBuf> {
    let mut dirs = BTreeSet::new();
    for dir in DEFAULT_LIBRARY_DIRS {
        let path = PathBuf::from(dir);
        if path.exists() {
            dirs.insert(path);
        }
    }

    for path in flag_paths("NIX_LDFLAGS", "-L") {
        if contains_native_library(&path) {
            dirs.insert(path);
        }
    }
    for package in NATIVE_LIBRARIES
        .iter()
        .filter_map(|library| library.pkg_config_package)
    {
        for path in pkg_config_library_dirs(package) {
            dirs.insert(path);
        }
    }

    dirs.into_iter().collect()
}

fn contains_native_library(dir: &Path) -> bool {
    NATIVE_LIBRARIES
        .iter()
        .any(|library| dir.join(library.file_name).exists())
}

fn pkg_config_library_dirs(package: &str) -> Vec<PathBuf> {
    pkg_config_paths(package, "--libs", &["-L"])
}

fn kconfig_compiler() -> PathBuf {
    if let Some(path) = command_in_path("kconfig_compiler_kf6") {
        return path;
    }

    for candidate in kconfig_compiler_candidates() {
        if candidate.exists() {
            return candidate;
        }
    }

    PathBuf::from("kconfig_compiler_kf6")
}

fn command_in_path(command: &str) -> Option<PathBuf> {
    let path = env::var_os("PATH")?;
    for dir in env::split_paths(&path) {
        let candidate = dir.join(command);
        if candidate.exists() {
            return Some(candidate);
        }
    }

    None
}

fn kconfig_compiler_candidates() -> Vec<PathBuf> {
    let mut candidates = Vec::new();
    for root in [PathBuf::from("/usr"), PathBuf::from("/app")] {
        candidates.push(root.join("libexec/kf6/kconfig_compiler_kf6"));
    }
    for lib_dir in DEFAULT_LIBRARY_DIRS {
        candidates.push(PathBuf::from(lib_dir).join("libexec/kf6/kconfig_compiler_kf6"));
    }

    for lib_dir in flag_paths("NIX_LDFLAGS", "-L") {
        if let Some(root) = lib_dir.parent() {
            candidates.push(root.join("libexec/kf6/kconfig_compiler_kf6"));
        }
    }

    candidates
}

fn flag_paths(env_var: &str, flag: &str) -> Vec<PathBuf> {
    let value = env::var(env_var).unwrap_or_default();
    flag_paths_from_tokens(value.split_whitespace(), &[flag])
}

fn flag_paths_from_tokens<'a>(
    tokens: impl IntoIterator<Item = &'a str>,
    flags: &[&str],
) -> Vec<PathBuf> {
    let mut paths = Vec::new();
    let mut tokens = tokens.into_iter();

    while let Some(token) = tokens.next() {
        if flags.contains(&token) {
            if let Some(path) = tokens.next() {
                paths.push(PathBuf::from(path));
            }
        } else if let Some(path) = joined_flag_path(token, flags) {
            paths.push(PathBuf::from(path));
        }
    }

    paths
}

fn joined_flag_path<'a>(token: &'a str, flags: &[&str]) -> Option<&'a str> {
    for flag in flags {
        if let Some(path) = token.strip_prefix(flag)
            && !path.is_empty()
        {
            return Some(path);
        }
    }

    None
}
