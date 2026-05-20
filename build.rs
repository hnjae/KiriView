// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_build::{CppFile, CxxQtBuilder, QmlModule};
use std::{
    collections::BTreeSet,
    env, fs,
    path::{Path, PathBuf},
    process::Command,
};

type IncludeDirCollector = fn(&mut BTreeSet<PathBuf>, &Path);

struct IncludeSearch {
    collectors: &'static [IncludeDirCollector],
    pkg_config_packages: &'static [&'static str],
}

const CPP_CORE_SOURCES_FILE: &str = "src/cpp_core_sources.txt";
const KCONFIG_SCHEMA_FILE: &str = "src/kiriviewstate.kcfg";
const KCONFIG_COMPILER_FILE: &str = "src/kiriviewstate.kcfgc";
const QML_SOURCE_DIR: &str = "src/qml";
const DEFAULT_INCLUDE_ROOTS: &[&str] = &["/usr/include"];
const DEFAULT_LIBRARY_DIRS: &[&str] = &["/usr/lib/x86_64-linux-gnu", "/usr/lib"];
const CXX_QT_HEADER_SOURCES: &[&str] = &[
    "src/application/menuaccesskeyrouter.h",
    "src/imageactionavailability.h",
    "src/kiriimagedocument.h",
    "src/kiriimageview.h",
    "src/imageshortcutnavigationpolicy.h",
    "src/kiriviewapplication.h",
    "src/powerprofilemonitor.h",
];
const RUST_BRIDGE_SOURCES: &[&str] = &[
    "src/applicationruntime.rs",
    "src/archiveformat.rs",
    "src/archivepath.rs",
    "src/avifcompat.rs",
    "src/cachebudget.rs",
    "src/heifcontainer.rs",
    "src/imagedocumentsourceloadpolicy.rs",
    "src/imageformatregistry.rs",
    "src/imageinputclassification.rs",
    "src/imageopenworkflow.rs",
    "src/imagerendergeometry.rs",
    "src/imagerotation.rs",
    "src/imagespreadgeometry.rs",
    "src/imagespreadnavigation.rs",
    "src/imagetilegeometry.rs",
    "src/imagenavigationmodel.rs",
    "src/imageviewportgeometry.rs",
    "src/imagezoomstate.rs",
    "src/predecodecachepolicy.rs",
    "src/svgrenderer.rs",
];
const CXX_QT_CPP_SOURCES: &[&str] = &[
    "src/kiriimagedocument.cpp",
    "src/kiriimagedecoder.cpp",
    "src/rendering/kiriimagerendernode.cpp",
    "src/kiriimageview.cpp",
    "src/kiriviewapplication.cpp",
];
const QT_MODULES: &[&str] = &[
    "Gui",
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
        link_name: "KF6KIOWidgets",
        file_name: "libKF6KIOWidgets.so",
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
        link_name: "raw",
        file_name: "libraw.so",
        pkg_config_package: Some("libraw"),
    },
    NativeLibrary {
        link_name: "png16",
        file_name: "libpng16.so",
        pkg_config_package: Some("libpng"),
    },
];
const NO_PKG_CONFIG_PACKAGES: &[&str] = &[];
const KIO_INCLUDE_COLLECTORS: &[IncludeDirCollector] =
    &[add_kf6_include_dirs, add_qt_mkspec_include_dirs];
const QT_RHI_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_qt_rhi_include_dirs];
const QT_QML_INTEGRATION_INCLUDE_COLLECTORS: &[IncludeDirCollector] =
    &[add_qt_qml_integration_include_dirs];
const LIBARCHIVE_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libarchive_include_dir];
const LIBHEIF_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libheif_include_dir];
const LIBRAW_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libraw_include_dir];
const LIBPNG_INCLUDE_COLLECTORS: &[IncludeDirCollector] = &[add_libpng_include_dir];
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
        collectors: LIBRAW_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libraw"],
    },
    IncludeSearch {
        collectors: LIBPNG_INCLUDE_COLLECTORS,
        pkg_config_packages: &["libpng"],
    },
    IncludeSearch {
        collectors: QT_RHI_INCLUDE_COLLECTORS,
        pkg_config_packages: &["Qt6Gui"],
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
    let shader_source = bake_shaders();
    let cpp_core_sources = cpp_core_sources();
    let generated_state = generate_kconfig_state();
    let generated_state_source = generated_state.source.to_string_lossy().into_owned();
    let qml_module = qml_module();
    link_native_libraries();

    let mut builder = CxxQtBuilder::new_qml_module(qml_module)
        // Do not export the whole crate root as C++ headers. That makes Cargo watch
        // build artifacts such as build-dir/, including symlinks into /run.
        .crate_include_root(None);

    builder = add_cpp_files(builder, CXX_QT_HEADER_SOURCES);
    builder = builder.cpp_file(CppFile::from(generated_state_source.as_str()));
    builder = add_rust_files(builder, RUST_BRIDGE_SOURCES);
    builder = add_cpp_files(builder, CXX_QT_CPP_SOURCES);
    builder = add_cpp_file_strings(builder, cpp_core_sources);
    builder = add_qt_modules(builder, QT_MODULES);

    unsafe {
        builder = builder.cc_builder(move |cc| {
            cc.flag_if_supported("-Wno-attributes");
            cc.define("TRANSLATION_DOMAIN", "\"kiriview\"");
            cc.file(&shader_source);
            cc.include("src");
            cc.include(&generated_state.include_dir);
            for dir in &native_include_dirs {
                cc.include(dir);
            }
        });
    }

    builder.build();
}

fn add_cpp_files(mut builder: CxxQtBuilder, files: &[&str]) -> CxxQtBuilder {
    for file in files {
        builder = builder.cpp_file(CppFile::from(*file));
    }
    builder
}

fn add_cpp_file_strings(mut builder: CxxQtBuilder, files: Vec<String>) -> CxxQtBuilder {
    for file in files {
        builder = builder.cpp_file(CppFile::from(file.as_str()));
    }
    builder
}

fn add_rust_files(mut builder: CxxQtBuilder, files: &[&str]) -> CxxQtBuilder {
    for file in files {
        builder = builder.file(*file);
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
    let mut module = QmlModule::new("io.github.hnjae.kiriview")
        .depend("QtQuick")
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

fn cpp_core_sources() -> Vec<String> {
    println!("cargo::rerun-if-changed={CPP_CORE_SOURCES_FILE}");
    fs::read_to_string(CPP_CORE_SOURCES_FILE)
        .expect("failed to read C++ core source list")
        .lines()
        .map(str::trim)
        .filter(|line| !line.is_empty() && !line.starts_with('#'))
        .map(str::to_owned)
        .collect()
}

fn bake_shaders() -> PathBuf {
    println!("cargo::rerun-if-changed=src/shaders/kiriimageview.vert");
    println!("cargo::rerun-if-changed=src/shaders/kiriimageview.frag");

    let out_dir = PathBuf::from(env::var_os("OUT_DIR").expect("OUT_DIR must be set"));
    let vertex_shader = out_dir.join("kiriimageview.vert.qsb");
    let fragment_shader = out_dir.join("kiriimageview.frag.qsb");

    run_qsb("src/shaders/kiriimageview.vert", &vertex_shader);
    run_qsb("src/shaders/kiriimageview.frag", &fragment_shader);

    let vertex_data = fs::read(&vertex_shader).expect("failed to read baked vertex shader");
    let fragment_data = fs::read(&fragment_shader).expect("failed to read baked fragment shader");
    let source = out_dir.join("kiriimageview_shaders.cpp");
    let mut generated = String::new();
    generated.push_str("// Generated by build.rs.\n\n");
    generated.push_str("#include \"shaders/kiriimageview_shaders.h\"\n\n");
    generated.push_str("namespace KiriView {\n");
    append_byte_array(
        &mut generated,
        "kiriImageViewVertexShader",
        "kiriImageViewVertexShaderSize",
        &vertex_data,
    );
    append_byte_array(
        &mut generated,
        "kiriImageViewFragmentShader",
        "kiriImageViewFragmentShaderSize",
        &fragment_data,
    );
    generated.push_str("}\n");
    fs::write(&source, generated).expect("failed to write generated shader source");
    source
}

fn run_qsb(input: &str, output: &Path) {
    let status = Command::new("qsb")
        .args(["--qt6", "-o"])
        .arg(output)
        .arg(input)
        .status()
        .expect("failed to run qsb; make sure Qt Shader Tools are installed");

    if !status.success() {
        panic!("qsb failed for {input}");
    }
}

fn append_byte_array(source: &mut String, name: &str, size_name: &str, data: &[u8]) {
    source.push_str("alignas(16) const unsigned char ");
    source.push_str(name);
    source.push_str("[] = {");
    for (index, byte) in data.iter().enumerate() {
        if index % 12 == 0 {
            source.push_str("\n    ");
        } else {
            source.push(' ');
        }
        source.push_str(&format!("0x{byte:02x},"));
    }
    source.push_str("\n};\nconst std::size_t ");
    source.push_str(size_name);
    source.push_str(" = sizeof(");
    source.push_str(name);
    source.push_str(");\n\n");
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

fn add_libraw_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("libraw").join("libraw.h").exists() {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_libpng_include_dir(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    if include_root.join("png.h").exists() {
        dirs.insert(include_root.to_path_buf());
    }
}

fn add_qt_rhi_include_dirs(dirs: &mut BTreeSet<PathBuf>, include_root: &Path) {
    for candidate in [
        include_root.to_path_buf(),
        include_root.join("QtGui"),
        include_root.join("QtGui").join(qt_version()).join("QtGui"),
        include_root.join(qt_version()).join("QtGui"),
    ] {
        if candidate.join("rhi/qrhi.h").exists() && candidate.join("rhi/qshader.h").exists() {
            dirs.insert(candidate);
        }
    }

    for dir in versioned_qt_gui_dirs(include_root) {
        if dir.join("rhi/qrhi.h").exists() && dir.join("rhi/qshader.h").exists() {
            dirs.insert(dir);
        }
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

fn versioned_qt_gui_dirs(include_root: &Path) -> Vec<PathBuf> {
    let mut dirs = Vec::new();
    for qt_gui_root in [include_root.to_path_buf(), include_root.join("QtGui")] {
        let Ok(entries) = fs::read_dir(qt_gui_root) else {
            continue;
        };
        for entry in entries.flatten() {
            let path = entry.path().join("QtGui");
            if path.exists() {
                dirs.push(path);
            }
        }
    }
    dirs
}

fn qt_version() -> String {
    Command::new("pkg-config")
        .args(["--modversion", "Qt6Gui"])
        .output()
        .ok()
        .filter(|output| output.status.success())
        .map(|output| String::from_utf8_lossy(&output.stdout).trim().to_owned())
        .filter(|version| !version.is_empty())
        .unwrap_or_else(|| "6".to_owned())
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
        "KF6/KIOWidgets",
        "KF6/KIO",
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
