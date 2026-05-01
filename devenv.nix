# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
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
in
{
  enterShell = ''
    export QMAKE=${lib.getExe' qmake "qmake6"}
  '';

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
    typos = {
      enable = true;
      settings.ignored-words = [ "INVOKABLE" ];
    };

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
