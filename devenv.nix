# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  imports = [
    ./devenv/modules/qt-cxxqt-dev.nix
    ./devenv/modules/check-tasks.nix
    ./devenv/modules/git-hooks.nix
    ./devenv/modules/i18n.nix
    ./devenv/modules/rust-host-toolchain.nix
    ./devenv/modules/treefmt.nix
  ];
}
