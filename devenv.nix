# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
let
  qtCxxqt = import ./nix/devenv/internal/qt-cxxqt.nix { inherit config pkgs lib; };
in
{
  imports = [
    ./nix/devenv/modules/check-tasks.nix
    ./nix/devenv/modules/git-hooks.nix
    ./nix/devenv/modules/i18n.nix
    ./nix/devenv/modules/treefmt.nix
  ];

  # Cargo debug builds compile the C++ bridge sources without optimization,
  # which makes glibc's fortify headers emit one warning per translation unit.
  hardeningDisable = [
    "fortify"
    "fortify3"
  ];

  enterShell = qtCxxqt.enterShell;

  files."compile_commands.json".json = qtCxxqt.compileCommands;
  files.".vscode/settings.json".json = qtCxxqt.vscodeSettings;
  files.".qmlls.ini".ini.General = qtCxxqt.qmllsGeneral;

  packages = [
    qtCxxqt.qmake
  ]
  ++ (with pkgs; [
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
    libpng
    lld
    ninja
    pkg-config
  ]);

  languages.rust.enable = true;
  languages.nix.enable = true;
  languages.cplusplus = {
    enable = true;
    lsp.package = pkgs.clang-tools;
  };
}
