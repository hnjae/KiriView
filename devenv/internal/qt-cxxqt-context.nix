# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
# Shared Qt/CXX-Qt tooling context consumed by devenv modules.
{
  config,
  pkgs,
  lib,
  karchivePackage ? pkgs.kdePackages.karchive,
}:
let
  qtToolPrefix = pkgs.symlinkJoin {
    name = "kiriview-qt-tools";
    paths = [
      karchivePackage
    ]
    ++ (with pkgs.kdePackages; [
      kconfig
      kimageformats
      ki18n
      kjobwidgets
      kirigami-addons
      kservice
      qtbase
      qtdeclarative
      qtimageformats
      qtmultimedia
      (qtmultimedia.dev or qtmultimedia)
      qtsvg
    ]);
  };
  qmake = pkgs.lib.hiPrio (
    pkgs.writeShellScriptBin "qmake6" ''
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
    ''
  );
  appQmlRoot = "${config.devenv.root}/target/cxxqt/qml_modules";
  qtQmlRoot = "${pkgs.kdePackages.qtdeclarative}/lib/qt-6/qml";
  qtmultimediaQmlRoot = "${pkgs.kdePackages.qtmultimedia}/lib/qt-6/qml";
  kconfigQmlRoot = "${pkgs.kdePackages.kconfig}/lib/qt-6/qml";
  ki18nQmlRoot = "${pkgs.kdePackages.ki18n}/lib/qt-6/qml";
  kirigamiQmlRoot = "${pkgs.kdePackages.kirigami.unwrapped}/lib/qt-6/qml";
  kirigamiAddonsQmlRoot = "${pkgs.kdePackages.kirigami-addons}/lib/qt-6/qml";
  qmlImportPaths = [
    appQmlRoot
    qtQmlRoot
    qtmultimediaQmlRoot
    kconfigQmlRoot
    ki18nQmlRoot
    kirigamiQmlRoot
    kirigamiAddonsQmlRoot
  ];
  hostRuntimeLibraryPath = lib.concatStringsSep ":" [
    "${config.devenv.root}/.devenv/profile/lib"
    (lib.makeLibraryPath [
      (lib.getLib pkgs.pipewire)
      (lib.getLib pkgs.stdenv.cc.cc)
    ])
  ];
  qtPluginPath = "${config.devenv.root}/.devenv/profile/lib/qt-6/plugins";
  qtVersion = lib.getVersion pkgs.kdePackages.qtbase;
  readSourceManifest =
    path:
    lib.filter (source: source != "" && !(lib.hasPrefix "#" source)) (
      lib.splitString "\n" (builtins.readFile path)
    );
  cppCoreSources = readSourceManifest ../../src/cpp_core_sources.txt;
  cxxQtCppSources = readSourceManifest ../../src/cpp_cxxqt_sources.txt;
  cppSources = lib.sort builtins.lessThan (cxxQtCppSources ++ cppCoreSources);
  cppTestSources = lib.sort builtins.lessThan (
    map (name: "tests/cpp/${name}") (
      lib.filter (name: lib.hasPrefix "test_" name && lib.hasSuffix ".cpp" name) (
        builtins.attrNames (builtins.readDir ../../tests/cpp)
      )
    )
  );
  qtCompileDefines = [
    "-DQT_CORE_LIB"
    "-DQT_DBUS_LIB"
    "-DQT_GUI_LIB"
    "-DQT_MULTIMEDIA_LIB"
    "-DQT_NETWORK_LIB"
    "-DQT_QML_LIB"
    "-DQT_QUICK_LIB"
    "-DQT_QUICKCONTROLS2_LIB"
    "-DQT_WIDGETS_LIB"
    "-DTRANSLATION_DOMAIN=\"kiriview\""
  ];
  gccVersion = lib.getVersion pkgs.stdenv.cc.cc;
  toolchainSystemIncludeDirs = [
    "${pkgs.stdenv.cc.cc}/include/c++/${gccVersion}"
    "${pkgs.stdenv.cc.cc}/include/c++/${gccVersion}/${pkgs.stdenv.hostPlatform.config}"
    "${pkgs.llvmPackages.clang}/resource-root/include"
    "${pkgs.glibc.dev}/include"
  ];
  qtIncludeModules = [
    "QtCore"
    "QtDBus"
    "QtGui"
    "QtMultimedia"
    "QtNetwork"
    "QtQml"
    "QtQmlIntegration"
    "QtQuick"
    "QtQuickControls2"
    "QtTest"
    "QtWidgets"
  ];
  systemIncludeDirs = [
    ".devenv/profile/include"
    ".devenv/profile/include/KF6/KArchive"
    ".devenv/profile/include/KF6/KConfig"
    ".devenv/profile/include/KF6/KConfigCore"
    ".devenv/profile/include/KF6/KConfigGui"
    ".devenv/profile/include/KF6/KCoreAddons"
    ".devenv/profile/include/KF6/KI18n"
    ".devenv/profile/include/KF6/KI18nQml"
    ".devenv/profile/include/KF6/KIO"
    ".devenv/profile/include/KF6/KIOCore"
    ".devenv/profile/include/KF6/KIOGui"
    ".devenv/profile/include/KF6/KIOWidgets"
    ".devenv/profile/include/KF6/KJobWidgets"
    ".devenv/profile/include/KF6/KService"
    ".devenv/profile/include/KirigamiAddonsStatefulApp"
    ".devenv/profile/include/QtGui/${qtVersion}/QtGui"
    ".devenv/profile/mkspecs/linux-g++"
  ]
  ++ toolchainSystemIncludeDirs
  ++ map (module: ".devenv/profile/include/${module}") qtIncludeModules;
  cppCompileCommand = source: extraArguments: {
    directory = config.devenv.root;
    file = source;
    arguments = [
      "clang++"
      "-std=c++17"
      "-Isrc"
      "-Itarget/cxxqt/clangd/include"
    ]
    ++ extraArguments
    ++ qtCompileDefines
    ++ lib.concatMap (dir: [
      "-isystem"
      dir
    ]) systemIncludeDirs
    ++ [ source ];
  };
  cppTestCompileCommand =
    source:
    let
      testTarget = lib.removeSuffix ".cpp" (baseNameOf source);
    in
    cppCompileCommand source [
      "-Itests/cpp"
      "-Itarget/devenv/cpp-tests/${testTarget}_autogen/include"
      "-DQT_TESTLIB_LIB"
      "-DKIRIVIEW_TEST_SOURCE_DIR=\"${config.devenv.root}/tests/cpp\""
    ];
  compileCommands =
    (map (source: cppCompileCommand source [ ]) cppSources)
    ++ (map cppTestCompileCommand cppTestSources);
  cppSourcesShellArgs = lib.escapeShellArgs cppSources;
  clazyIgnoreDirsRegex = "(^|/)(\\.devenv|target)(/|$)|^/nix/store/";
  qmlLintImportArgs = lib.escapeShellArgs (
    lib.concatMap (path: [
      "-I"
      path
    ]) qmlImportPaths
  );
  rustAnalyzerToml = # toml
    ''
      linkedProjects = ["Cargo.toml"]

      [cargo]
      targetDir = "target/rust-analyzer"

      [files]
      exclude = [
          ".cargo-vendor",
          ".devenv",
          ".direnv",
          ".flatpak-builder",
          ".flatpak-cargo",
          ".rumdl_cache",
          "build-dir",
          "repo",
          "target",
      ]
    '';
  refreshCxxqtIncludes = pkgs.writeShellApplication {
    name = "refresh-cxxqt-includes";
    runtimeInputs = with pkgs; [
      coreutils
      findutils
      util-linux
    ];
    text = ''
      set -euo pipefail

      repo_root=${lib.escapeShellArg config.devenv.root}
      cxxqt_clangd_include="$repo_root/target/cxxqt/clangd/include"
      lock_file="$repo_root/target/cxxqt/clangd/refresh.lock"

      mkdir -p "$cxxqt_clangd_include"
      exec 9>"$lock_file"
      flock 9

      find "$cxxqt_clangd_include" -mindepth 1 -maxdepth 1 -type l -delete

      link_generated_include_dir() {
          local generated_include_dir="$1"
          local generated_include
          local link_target

          if [[ ! -d "$generated_include_dir" ]]; then
              return
          fi

          while IFS= read -r -d "" generated_include; do
              link_target="$cxxqt_clangd_include/''${generated_include##*/}"
              rm -rf "$link_target"
              ln -sT "$generated_include" "$link_target"
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
  cppLintPrelude = # sh
  ''
    set -euo pipefail

    cd ${lib.escapeShellArg config.devenv.root}
    ${lib.getExe refreshCxxqtIncludes}

    if [[ ! -f compile_commands.json ]]; then
        echo "compile_commands.json was not found; enter the devenv shell to generate it" >&2
        exit 1
    fi
  ''
  +
    lib.optionalString (cppSources == [ ]) # sh
      ''
        echo "compile_commands.json does not contain any C++ sources" >&2
        exit 1
      '';
  qtBuildEnvironment = # sh
    ''
      export QMAKE=${lib.getExe' qmake "qmake6"}

      declare -A seen_cmake_prefix_paths=()
      cmake_prefix_path=""
      add_cmake_prefix_path() {
          local prefix_path=$1

          if [[ -z $prefix_path || ! -d $prefix_path || -v 'seen_cmake_prefix_paths[$prefix_path]' ]]; then
              return
          fi

          seen_cmake_prefix_paths[$prefix_path]=1
          cmake_prefix_path="''${cmake_prefix_path:+$cmake_prefix_path:}$prefix_path"
      }

      add_cmake_prefix_path ${lib.escapeShellArg "${config.devenv.root}/.devenv/profile"}
      IFS=: read -r -a existing_cmake_prefix_paths <<<"''${CMAKE_PREFIX_PATH:-}"
      for prefix_path in "''${existing_cmake_prefix_paths[@]}"; do
          add_cmake_prefix_path "$prefix_path"
      done
      export CMAKE_PREFIX_PATH="$cmake_prefix_path"
      unset seen_cmake_prefix_paths cmake_prefix_path prefix_path
      unset -f add_cmake_prefix_path
    '';
  qtRuntimeEnvironment = # sh
    ''
      ${qtBuildEnvironment}

      export QT_PLUGIN_PATH=${lib.escapeShellArg qtPluginPath}
      declare -A seen_runtime_library_paths=()
      runtime_library_path=""
      add_runtime_library_path() {
          local library_path=$1

          if [[ -z $library_path || ! -d $library_path || -v 'seen_runtime_library_paths[$library_path]' ]]; then
              return
          fi

          seen_runtime_library_paths[$library_path]=1
          runtime_library_path="''${runtime_library_path:+$runtime_library_path:}$library_path"
      }

      IFS=: read -r -a host_runtime_library_paths <<<${lib.escapeShellArg hostRuntimeLibraryPath}
      for library_path in "''${host_runtime_library_paths[@]}"; do
          add_runtime_library_path "$library_path"
      done

      # shellcheck disable=SC2206
      nix_ldflags=( ''${NIX_LDFLAGS:-} )
      for ((i = 0; i < ''${#nix_ldflags[@]}; i++)); do
          case "''${nix_ldflags[$i]}" in
          -L)
              ((i += 1))
              library_path="''${nix_ldflags[$i]:-}"
              ;;
          -L*)
              library_path="''${nix_ldflags[$i]#-L}"
              ;;
          *)
              continue
              ;;
          esac

          add_runtime_library_path "$library_path"
      done

      IFS=: read -r -a existing_runtime_library_paths <<<"''${LD_LIBRARY_PATH:-}"
      for library_path in "''${existing_runtime_library_paths[@]}"; do
          add_runtime_library_path "$library_path"
      done
      export LD_LIBRARY_PATH="$runtime_library_path"
      unset seen_runtime_library_paths runtime_library_path library_path
      unset -f add_runtime_library_path
    '';
in
{
  inherit
    compileCommands
    clazyIgnoreDirsRegex
    cppLintPrelude
    cppSourcesShellArgs
    qmake
    qmlLintImportArgs
    refreshCxxqtIncludes
    rustAnalyzerToml
    qtBuildEnvironment
    qtRuntimeEnvironment
    ;

  enterShell = # sh
    ''
      ${qtRuntimeEnvironment}

      "${lib.getExe refreshCxxqtIncludes}"
    '';

  qmllsGeneral = {
    buildDir = "${config.devenv.root}/target/cxxqt/qml_modules";
    importPaths = lib.concatStringsSep ":" qmlImportPaths;
    "no-cmake-calls" = true;
  };

}
