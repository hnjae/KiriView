# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
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
  karchiveDev = karchivePackage.dev or karchivePackage;
  kconfigDev = pkgs.kdePackages.kconfig.dev or pkgs.kdePackages.kconfig;
  kcoreaddonsDev = pkgs.kdePackages.kcoreaddons.dev or pkgs.kdePackages.kcoreaddons;
  ki18nDev = pkgs.kdePackages.ki18n.dev or pkgs.kdePackages.ki18n;
  kjobwidgetsDev = pkgs.kdePackages.kjobwidgets.dev or pkgs.kdePackages.kjobwidgets;
  kirigamiAddonsDev = pkgs.kdePackages.kirigami-addons.dev or pkgs.kdePackages.kirigami-addons;
  kioDev = pkgs.kdePackages.kio.dev or pkgs.kdePackages.kio;
  kserviceDev = pkgs.kdePackages.kservice.dev or pkgs.kdePackages.kservice;
  qtmultimediaDev = pkgs.kdePackages.qtmultimedia.dev or pkgs.kdePackages.qtmultimedia;
  libpngDev = pkgs.libpng.dev or pkgs.libpng;
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
  qtPluginPath = "${config.devenv.root}/.devenv/profile/lib/qt-6/plugins";
  qtVersion = lib.getVersion pkgs.kdePackages.qtbase;
  readSourceManifest =
    path:
    lib.filter (source: source != "" && !(lib.hasPrefix "#" source)) (
      lib.splitString "\n" (builtins.readFile path)
    );
  cppCoreSources = readSourceManifest ../../../src/cpp_core_sources.txt;
  cxxQtCppSources = readSourceManifest ../../../src/cpp_cxxqt_sources.txt;
  cppSources = lib.sort builtins.lessThan (cxxQtCppSources ++ cppCoreSources);
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
    "QtWidgets"
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
    ".devenv/profile/include/KF6/KIOGui"
    ".devenv/profile/include/KF6/KIOWidgets"
    ".devenv/profile/include/KF6/KJobWidgets"
    ".devenv/profile/include/KF6/KService"
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
    "${kioDev}/include/KF6/KIO"
    "${kioDev}/include/KF6/KIOCore"
    "${kioDev}/include/KF6/KIOGui"
    "${kioDev}/include/KF6/KIOWidgets"
    "${kjobwidgetsDev}/include/KF6/KJobWidgets"
    "${kserviceDev}/include/KF6/KService"
    "${libpngDev}/include/libpng16"
    "${pkgs.kdePackages.qtmultimedia}/include"
    "${pkgs.kdePackages.qtmultimedia}/include/QtMultimedia"
    "${qtmultimediaDev}/mkspecs/modules"
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
  clazyIgnoreDirsRegex = "(^|/)(\\.devenv|target)(/|$)|^/nix/store/";
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
  runtimeEnvironment = ''
    export CMAKE_PREFIX_PATH=${lib.escapeShellArg "${config.devenv.root}/.devenv/profile"}''${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}
    export QT_PLUGIN_PATH=${lib.escapeShellArg qtPluginPath}
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
    runtimeEnvironment
    ;

  enterShell = ''
    export QMAKE=${lib.getExe' qmake "qmake6"}
    ${runtimeEnvironment}

    "${lib.getExe refreshCxxqtIncludes}"
  '';

  qmllsGeneral = {
    buildDir = "${config.devenv.root}/target/cxxqt/qml_modules";
    importPaths = lib.concatStringsSep ":" qmlImportPaths;
    "no-cmake-calls" = true;
  };

  vscodeSettings = {
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
}
