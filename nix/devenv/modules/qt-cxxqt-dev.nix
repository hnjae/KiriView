# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
let
  karchiveContentChecksumPatch = ../../patches/karchive-content-checksum.patch;
  patchedKArchive = pkgs.kdePackages.karchive.overrideAttrs (old: {
    patches = (old.patches or [ ]) ++ [ karchiveContentChecksumPatch ];
  });
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
    karchivePackage = patchedKArchive;
  };
in
{
  _module.args = {
    inherit patchedKArchive qtCxxqt;
  };

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
    patchedKArchive
    kiriviewLibHeif.bin
    kiriviewLibHeif.dev
    kiriviewLibHeif.lib
  ]
  ++ (with pkgs; [
    # Flatpak
    desktop-file-utils
    flatpak-builder

    # Rust/Qt host development
    clazy
    cmake
    kdePackages.extra-cmake-modules
    kdePackages.kconfig
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
    libpng
    libraw.dev
    libraw.lib
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
