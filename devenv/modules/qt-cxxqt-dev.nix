# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  rustHostEnvironment,
  ...
}:
let
  kiriviewLibHeif = pkgs.libheif.overrideAttrs (old: {
    cmakeFlags = (old.cmakeFlags or [ ]) ++ [
      "-DWITH_JPEG_DECODER=ON"
      "-DWITH_JPEG_DECODER_PLUGIN=OFF"
      "-DWITH_JPEG_ENCODER=ON"
      "-DWITH_JPEG_ENCODER_PLUGIN=OFF"
    ];
  });
  qtCxxqt = import ../internal/qt-cxxqt-context.nix {
    inherit config lib pkgs;
    karchivePackage = pkgs.kdePackages.karchive;
  };
in
{
  # Shared Qt/CXX-Qt tooling context for the dev shell, editor metadata, and
  # check tasks. Keep cross-module qmake, QML import, and runtime env logic here.
  _module.args = {
    inherit qtCxxqt;
  };

  # Cargo debug builds compile the C++ bridge sources without optimization,
  # which makes glibc's fortify headers emit one warning per translation unit.
  hardeningDisable = [
    "fortify"
    "fortify3"
  ];

  enterShell = # sh
    ''
      ${qtCxxqt.enterShell}
      ${rustHostEnvironment}
    '';

  files."compile_commands.json".json = qtCxxqt.compileCommands;
  files."rust-analyzer.toml".text = qtCxxqt.rustAnalyzerToml;
  files.".qmlls.ini".ini.General = qtCxxqt.qmllsGeneral;

  packages = [
    qtCxxqt.qmake
    pkgs.kdePackages.karchive
    kiriviewLibHeif.bin
    kiriviewLibHeif.dev
    kiriviewLibHeif.lib

    # Flatpak
    pkgs.desktop-file-utils
    pkgs.flatpak-builder
    pkgs.jq

    # Rust/Qt host development
    pkgs.cargo-nextest
    pkgs.clazy
    pkgs.cmake
    pkgs.kdePackages.extra-cmake-modules
    pkgs.kdePackages.kconfig
    pkgs.kdePackages.kcoreaddons
    pkgs.kdePackages.kimageformats
    pkgs.kdePackages.ki18n
    pkgs.kdePackages.kjobwidgets
    pkgs.kdePackages.kirigami
    pkgs.kdePackages.kirigami-addons
    pkgs.kdePackages.kio
    pkgs.kdePackages.kservice
    pkgs.kdePackages.kio-extras
    pkgs.kdePackages.kio-fuse
    pkgs.kdePackages.qqc2-desktop-style
    pkgs.kdePackages.qtbase
    pkgs.kdePackages.qtdeclarative
    pkgs.kdePackages.qtimageformats
    pkgs.kdePackages.qtmultimedia
    pkgs.kdePackages.qtsvg
    pkgs.kdePackages.qttools
    pkgs.libarchive
    pkgs.libaom
    pkgs.libde265
    pkgs.libjxl.dev
    pkgs.libjxl.out
    pkgs.libraw.dev
    pkgs.libraw.lib
    pkgs.libwebp
    pkgs.ninja
    pkgs.pipewire
    pkgs.pkg-config
    pkgs.x265
  ];

  languages.rust.enable = true;
  languages.nix.enable = true;
  languages.cplusplus = {
    enable = true;
    lsp.package = pkgs.clang-tools;
  };
}
