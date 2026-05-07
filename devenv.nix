# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
let
  qtToolPrefix = pkgs.symlinkJoin {
    name = "kiriview-qt-tools";
    paths = with pkgs.kdePackages; [
      karchive
      kconfig
      kimageformats
      ki18n
      kirigami-addons
      qtbase
      qtdeclarative
      qtimageformats
      qtsvg
    ];
  };
  qmake = pkgs.writeShellScriptBin "qmake6" ''
    if [ "$#" -eq 2 ] && [ "$1" = "-query" ]; then
      case "$2" in
        QT_INSTALL_PREFIX|QT_INSTALL_PREFIX/get|QT_HOST_PREFIX|QT_HOST_PREFIX/get|QT_INSTALL_ARCHDATA|QT_INSTALL_ARCHDATA/get|QT_INSTALL_DATA|QT_INSTALL_DATA/get|QT_HOST_DATA|QT_HOST_DATA/get)
          printf '%s\n' '${qtToolPrefix}'
          exit 0
          ;;
        QT_INSTALL_HEADERS|QT_INSTALL_HEADERS/get)
          printf '%s\n' '${qtToolPrefix}/include'
          exit 0
          ;;
        QT_INSTALL_LIBS|QT_INSTALL_LIBS/get|QT_HOST_LIBS|QT_HOST_LIBS/get)
          printf '%s\n' '${qtToolPrefix}/lib'
          exit 0
          ;;
        QT_HOST_LIBEXECS|QT_HOST_LIBEXECS/get|QT_INSTALL_LIBEXECS|QT_INSTALL_LIBEXECS/get)
          printf '%s\n' '${qtToolPrefix}/libexec'
          exit 0
          ;;
        QT_HOST_BINS|QT_HOST_BINS/get|QT_INSTALL_BINS|QT_INSTALL_BINS/get)
          printf '%s\n' '${qtToolPrefix}/bin'
          exit 0
          ;;
        QT_INSTALL_PLUGINS|QT_INSTALL_PLUGINS/get)
          printf '%s\n' '${qtToolPrefix}/lib/qt-6/plugins'
          exit 0
          ;;
        QT_INSTALL_QML|QT_INSTALL_QML/get)
          printf '%s\n' '${qtToolPrefix}/lib/qt-6/qml'
          exit 0
          ;;
      esac
    fi

    exec ${lib.getExe' pkgs.kdePackages.qtbase "qmake6"} "$@"
  '';
  cxxCompiler = pkgs.stdenv.cc.cc;
  cxxStandardLibraryVersion = lib.getVersion cxxCompiler;
  cxxTarget = pkgs.stdenv.hostPlatform.config;
  karchiveDev = pkgs.kdePackages.karchive.dev or pkgs.kdePackages.karchive;
  kconfigDev = pkgs.kdePackages.kconfig.dev or pkgs.kdePackages.kconfig;
  kcoreaddonsDev = pkgs.kdePackages.kcoreaddons.dev or pkgs.kdePackages.kcoreaddons;
  ki18nDev = pkgs.kdePackages.ki18n.dev or pkgs.kdePackages.ki18n;
  kirigamiAddonsDev = pkgs.kdePackages.kirigami-addons.dev or pkgs.kdePackages.kirigami-addons;
  appQmlRoot = "${config.devenv.root}/target/cxxqt/qml_modules";
  qtQmlRoot = "${config.devenv.root}/.devenv/profile/lib/qt-6/qml";
  kconfigQmlRoot = "${pkgs.kdePackages.kconfig}/lib/qt-6/qml";
  ki18nQmlRoot = "${pkgs.kdePackages.ki18n}/lib/qt-6/qml";
  kirigamiQmlRoot = "${pkgs.kdePackages.kirigami.unwrapped}/lib/qt-6/qml";
  kirigamiAddonsQmlRoot = "${pkgs.kdePackages.kirigami-addons}/lib/qt-6/qml";
  qmlImportPaths = [
    appQmlRoot
    qtQmlRoot
    kconfigQmlRoot
    ki18nQmlRoot
    kirigamiQmlRoot
    kirigamiAddonsQmlRoot
  ];
  qtVersion = lib.getVersion pkgs.kdePackages.qtbase;
  srcEntries = builtins.readDir ./src;
  cppSources = map (source: "src/${source}") (
    lib.sort builtins.lessThan (
      lib.filter (source: srcEntries.${source} == "regular" && lib.hasSuffix ".cpp" source) (
        builtins.attrNames srcEntries
      )
    )
  );
  qtCompileDefines = [
    "-DQT_CORE_LIB"
    "-DQT_DBUS_LIB"
    "-DQT_GUI_LIB"
    "-DQT_NETWORK_LIB"
    "-DQT_QML_LIB"
    "-DQT_QUICK_LIB"
    "-DQT_SVG_LIB"
    "-DTRANSLATION_DOMAIN=\"kiriview\""
  ];
  qtIncludeModules = [
    "QtCore"
    "QtDBus"
    "QtGui"
    "QtNetwork"
    "QtQml"
    "QtQmlIntegration"
    "QtQuick"
    "QtSvg"
  ];
  cxxStandardLibraryIncludeDirs = [
    "${cxxCompiler}/include/c++/${cxxStandardLibraryVersion}"
    "${cxxCompiler}/include/c++/${cxxStandardLibraryVersion}/${cxxTarget}"
    "${pkgs.stdenv.cc.libc_dev}/include"
  ];
  systemIncludeDirs = [
    ".devenv/profile/include"
    ".devenv/profile/include/KF6/KArchive"
    ".devenv/profile/include/KF6/KConfig"
    ".devenv/profile/include/KF6/KConfigCore"
    ".devenv/profile/include/KF6/KConfigGui"
    ".devenv/profile/include/KF6/KI18n"
    ".devenv/profile/include/KF6/KI18nQml"
    ".devenv/profile/include/KF6/KIO"
    ".devenv/profile/include/KF6/KIOCore"
    ".devenv/profile/include/KirigamiAddonsStatefulApp"
    ".devenv/profile/include/QtGui/${qtVersion}/QtGui"
    ".devenv/profile/mkspecs/linux-g++"
    "${karchiveDev}/include/KF6/KArchive"
    "${kconfigDev}/include/KF6/KConfig"
    "${kconfigDev}/include/KF6/KConfigCore"
    "${kconfigDev}/include/KF6/KConfigGui"
    "${kcoreaddonsDev}/include/KF6/KCoreAddons"
    "${ki18nDev}/include/KF6/KI18n"
    "${ki18nDev}/include/KF6/KI18nQml"
    "${kirigamiAddonsDev}/include/KirigamiAddonsStatefulApp"
  ]
  ++ cxxStandardLibraryIncludeDirs
  ++ map (module: ".devenv/profile/include/${module}") qtIncludeModules;
  compileCommands = map (source: {
    directory = config.devenv.root;
    file = source;
    arguments = [
      "clang++"
      "-std=c++17"
      "-Isrc"
      "-Itarget/cxxqt/clangd/include"
    ]
    ++ qtCompileDefines
    ++ lib.concatMap (dir: [
      "-isystem"
      dir
    ]) systemIncludeDirs
    ++ [ source ];
  }) cppSources;
  cppSourcesShellArgs = lib.escapeShellArgs cppSources;
  qmlLintImportArgs = lib.escapeShellArgs (
    lib.concatMap (path: [
      "-I"
      path
    ]) qmlImportPaths
  );
  refreshCxxqtIncludes = pkgs.writeShellApplication {
    name = "refresh-cxxqt-includes";
    runtimeInputs = with pkgs; [
      coreutils
      findutils
    ];
    text = ''
      set -euo pipefail

      repo_root=${lib.escapeShellArg config.devenv.root}
      cxxqt_clangd_include="$repo_root/target/cxxqt/clangd/include"

      mkdir -p "$cxxqt_clangd_include"
      find "$cxxqt_clangd_include" -mindepth 1 -maxdepth 1 -type l -delete

      link_generated_include_dir() {
          local generated_include_dir="$1"
          local generated_include

          if [[ ! -d "$generated_include_dir" ]]; then
              return
          fi

          while IFS= read -r -d "" generated_include; do
              ln -sfn "$generated_include" "$cxxqt_clangd_include/''${generated_include##*/}"
          done < <(find "$generated_include_dir" -mindepth 1 -maxdepth 1 -print0)
      }

      while IFS= read -r generated_include_dir; do
          link_generated_include_dir "$generated_include_dir"
      done < <(
          find \
              "$repo_root"/target/rust-analyzer \
              "$repo_root"/target/devenv \
              "$repo_root"/target/flatpak \
              "$repo_root"/target/release \
              "$repo_root"/target/debug \
              \( -path "*/out/cxxqtbuild/include" -o -path "*/out/kconfig" \) \
              -type d \
              -printf "%T@ %p\n" 2>/dev/null \
              | sort -n \
              | sed "s/^[^ ]* //"
      )
    '';
  };
  cppLintPrelude = ''
    set -euo pipefail

    cd ${lib.escapeShellArg config.devenv.root}
    ${lib.getExe refreshCxxqtIncludes}

    if [[ ! -f compile_commands.json ]]; then
        echo "compile_commands.json was not found; enter the devenv shell to generate it" >&2
        exit 1
    fi
  ''
  + lib.optionalString (cppSources == [ ]) ''
    echo "compile_commands.json does not contain any C++ sources" >&2
    exit 1
  '';
in
{
  # Cargo debug builds compile the C++ bridge sources without optimization,
  # which makes glibc's fortify headers emit one warning per translation unit.
  hardeningDisable = [
    "fortify"
    "fortify3"
  ];

  enterShell = ''
    export QMAKE=${lib.getExe' qmake "qmake6"}

    "${lib.getExe refreshCxxqtIncludes}"
  '';

  files."compile_commands.json".json = compileCommands;

  files.".vscode/settings.json".json = {
    "rust-analyzer.linkedProjects" = [ "Cargo.toml" ];
    "rust-analyzer.cargo.targetDir" = "target/rust-analyzer";
    "rust-analyzer.files.exclude" = [
      ".cargo-vendor"
      ".devenv"
      ".direnv"
      ".flatpak-builder"
      ".flatpak-cargo"
      ".rumdl_cache"
      "build-dir"
      "repo"
      "target"
    ];
  };

  files.".qmlls.ini".ini.General = {
    buildDir = "${config.devenv.root}/target/cxxqt/qml_modules";
    importPaths = lib.concatStringsSep ":" qmlImportPaths;
    "no-cmake-calls" = true;
  };

  scripts = {
    "test-rust-host" = {
      description = "Run host Rust library tests";
      exec = ''
        set -euo pipefail

        cd ${lib.escapeShellArg config.devenv.root}

        CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target"} \
            cargo test --locked --lib --all-features
      '';
    };
    "test-cpp-host" = {
      description = "Run host C++ tests against host Rust artifacts";
      exec = ''
        set -euo pipefail

        cd ${lib.escapeShellArg config.devenv.root}

        cmake \
            -S tests/cpp \
            -B target/devenv/cpp-tests \
            -DCMAKE_BUILD_TYPE=Debug \
            -DKIRIVIEW_CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target/debug"}
        cmake --build target/devenv/cpp-tests
        # GNU gettext ignores LANGUAGE under C/POSIX locales; devenv defaults to C.UTF-8.
        LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 \
            ctest \
                --test-dir target/devenv/cpp-tests \
                --output-on-failure \
                -E '^test_kiriimagedecoder$'
      '';
    };
    "test-host" = {
      description = "Run host Rust and C++ tests";
      exec = ''
        set -euo pipefail

        test-rust-host
        test-cpp-host
      '';
    };
    "lint-clippy" = {
      description = "Run Rust clippy";
      exec = ''
        set -euo pipefail

        cd ${lib.escapeShellArg config.devenv.root}

        cargo clippy --locked --all-targets --all-features -- -D warnings
      '';
    };
    "lint-qmllint" = {
      description = "Run qmllint against QML sources";
      exec = ''
        set -euo pipefail

        cd ${lib.escapeShellArg config.devenv.root}
        ${lib.getExe' pkgs.kdePackages.qtdeclarative "qmllint"} ${qmlLintImportArgs} --ignore-settings --max-warnings 0 src/qml/*.qml
      '';
    };
    "lint-cpp" = {
      description = "Run C++ linters";
      exec = ''
        ${cppLintPrelude}
        ${lib.getExe' pkgs.clang-tools "clang-tidy"} --quiet -p . ${cppSourcesShellArgs}
        ${lib.getExe' pkgs.clazy "clazy-standalone"} --checks="''${CLAZY_CHECKS:-level0}" -p . ${cppSourcesShellArgs}
      '';
    };
  };

  packages = with pkgs; [
    qmake

    # Flatpak
    desktop-file-utils
    flatpak-builder

    # Rust/Qt host development
    clazy
    cmake
    kdePackages.extra-cmake-modules
    kdePackages.karchive
    kdePackages.kconfig
    kdePackages.kimageformats
    kdePackages.ki18n
    kdePackages.kirigami
    kdePackages.kirigami-addons
    kdePackages.kio
    kdePackages.qqc2-desktop-style
    kdePackages.qtbase
    kdePackages.qtdeclarative
    kdePackages.qtimageformats
    kdePackages.qtsvg
    kdePackages.qttools
    libarchive
    libheif.dev
    libheif.lib
    ninja
    pkg-config
  ];

  languages.rust.enable = true;
  languages.nix.enable = true;
  languages.cplusplus = {
    enable = true;
    lsp.package = pkgs.clang-tools;
  };
}
