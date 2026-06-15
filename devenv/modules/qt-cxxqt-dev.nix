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
  qtCxxqt = import ../internal/qt-cxxqt.nix {
    inherit config lib pkgs;
    karchivePackage = pkgs.kdePackages.karchive;
  };
in
{
  _module.args = {
    inherit qtCxxqt;
  };

  # Cargo debug builds compile the C++ bridge sources without optimization,
  # which makes glibc's fortify headers emit one warning per translation unit.
  hardeningDisable = [
    "fortify"
    "fortify3"
  ];

  enterShell = ''
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
  ]
  ++ (with pkgs; [
    # Flatpak
    desktop-file-utils
    flatpak-builder
    jq

    # Rust/Qt host development
    cargo-nextest
    clazy
    cmake
    kdePackages.extra-cmake-modules
    kdePackages.kconfig
    kdePackages.kcoreaddons
    kdePackages.kimageformats
    kdePackages.ki18n
    kdePackages.kjobwidgets
    kdePackages.kirigami
    kdePackages.kirigami-addons
    kdePackages.kio
    kdePackages.kservice
    kdePackages.kio-extras
    kdePackages.kio-fuse
    kdePackages.qqc2-desktop-style
    kdePackages.qtbase
    kdePackages.qtdeclarative
    kdePackages.qtimageformats
    kdePackages.qtmultimedia
    kdePackages.qtsvg
    kdePackages.qttools
    libarchive
    libaom
    libde265
    libjxl.dev
    libjxl.out
    libraw.dev
    libraw.lib
    libwebp
    ninja
    pipewire
    pkg-config
    x265
  ]);

  languages.rust.enable = true;
  languages.nix.enable = true;
  languages.cplusplus = {
    enable = true;
    lsp.package = pkgs.clang-tools;
  };
}
