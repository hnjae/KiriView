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
      kimageformats
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
  kcoreaddonsDev = pkgs.kdePackages.kcoreaddons.dev or pkgs.kdePackages.kcoreaddons;
  qtVersion = lib.getVersion pkgs.kdePackages.qtbase;
  cppSources = [
    "src/apngdecoder.cpp"
    "src/asyncobjectslot.cpp"
    "src/imageanimationplayer.cpp"
    "src/imageformatregistry.cpp"
    "src/imageloader.cpp"
    "src/imagepredecodecoordinator.cpp"
    "src/imagezoomstate.cpp"
    "src/imagenavigationservice.cpp"
    "src/kiriimagedecoder.cpp"
    "src/kiriimagenavigation.cpp"
    "src/kiriimagerendernode.cpp"
    "src/kiriimageview.cpp"
    "src/predecodecache.cpp"
  ];
  qtCompileDefines = [
    "-DQT_CORE_LIB"
    "-DQT_DBUS_LIB"
    "-DQT_GUI_LIB"
    "-DQT_NETWORK_LIB"
    "-DQT_QML_LIB"
    "-DQT_QUICK_LIB"
    "-DQT_SVG_LIB"
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
    ".devenv/profile/include/KF6/KIO"
    ".devenv/profile/include/KF6/KIOCore"
    ".devenv/profile/include/QtGui/${qtVersion}/QtGui"
    ".devenv/profile/mkspecs/linux-g++"
    "${kcoreaddonsDev}/include/KF6/KCoreAddons"
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
in
{
  enterShell = ''
    export QMAKE=${lib.getExe' qmake "qmake6"}

    cxxqt_clangd_include="${config.devenv.root}/target/cxxqt/clangd/include"
    mkdir -p "$cxxqt_clangd_include"
    for cxxqt_include in \
      "${config.devenv.root}"/target/debug/build/*/out/cxxqtbuild/include \
      "${config.devenv.root}"/target/devenv/debug/build/*/out/cxxqtbuild/include \
      "${config.devenv.root}"/target/rust-analyzer/debug/build/*/out/cxxqtbuild/include; do
      if [ -d "$cxxqt_include" ]; then
        find "$cxxqt_include" -mindepth 1 -maxdepth 1 -exec ln -sfn {} "$cxxqt_clangd_include/" \;
      fi
    done
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
    "no-cmake-calls" = true;
  };

  packages = with pkgs; [
    qmake

    # Flatpak
    flatpak-builder

    # Rust/Qt host development
    clazy
    cmake
    kdePackages.extra-cmake-modules
    kdePackages.kimageformats
    kdePackages.kirigami
    kdePackages.kio
    kdePackages.qqc2-desktop-style
    kdePackages.qtbase
    kdePackages.qtdeclarative
    kdePackages.qtimageformats
    kdePackages.qtsvg
    kdePackages.qttools
    ninja
    pkg-config
  ];

  languages.rust.enable = true;
  languages.nix.enable = true;
  languages.cplusplus = {
    enable = true;
    lsp.package = pkgs.clang-tools;
  };

  git-hooks.excludes = [
    "devenv\.lock"
  ];
  git-hooks.hooks = {
    # Static checkers
    detect-private-keys.enable = true;
    editorconfig-checker.enable = true;
    gitlint.enable = true;
    reuse.enable = true;
    typos.enable = true;

    # Rust
    rustfmt.enable = true;

    # QML
    qmlformat = {
      enable = true;
      package = pkgs.runCommandLocal "qmlformat" { } ''
        mkdir -p "$out/bin"
        ln -s "${lib.getExe' pkgs.kdePackages.qtdeclarative "qmlformat"}" "$out/bin/qmlformat"
      '';
      files = ''\.qml$'';
      entry = "qmlformat -i";
    };

    # C++
    clang-format = {
      enable = true;
      package = pkgs.clang-tools;
      files = ''\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$'';
      entry = "clang-format -i --style=file";
    };

    # Nix
    deadnix.enable = true;
    nixfmt.enable = true;
    statix.enable = true;

    # Miscellaneous Formatters
    rumdl.enable = true;
    biome.enable = true;
    taplo.enable = true;
    yamlfmt.enable = true;
    shell-format = {
      enable = true;
      package = pkgs.symlinkJoin {
        name = "shell-format";
        paths = with pkgs; [
          shellharden
          shellcheck
          shfmt
        ];
      };
      files = ''
        (?x)^(
          .*\.(sh|bash)$|
          \.envrc(\..+)?$|
          \.env(\..+)?$
        )
      '';
      entry = toString (
        pkgs.writeScript "precommit-shell-format"
          # sh
          ''
            #!${pkgs.dash}/bin/dash
            set -eu

            for file in "$@"; do
              shellharden --replace "$file"
              shellcheck "$file"
              shfmt --indent 4 --simplify --write "$file"
            done
          ''
      );
    };
    just-format = {
      enable = true;
      name = "just-fmt";
      files = ''(^|/)(\.)?[jJ]ustfile$'';
      entry = toString (
        pkgs.writeScript "precommit-just-fmt"
          # sh
          ''
            #!${pkgs.dash}/bin/dash

            set -eu

            for file in "$@"; do
              ${lib.getExe pkgs.just} --unstable --fmt --justfile "$file"
            done
          ''
      );
    };
  };
}
