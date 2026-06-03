# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{ ... }:
{
  imports = [
    ./nix/devenv/modules/qt-cxxqt-dev.nix
    ./nix/devenv/modules/check-tasks.nix
    ./nix/devenv/modules/git-hooks.nix
    ./nix/devenv/modules/i18n.nix
    ./nix/devenv/modules/rust-host-toolchain.nix
    ./nix/devenv/modules/treefmt.nix
  ];
}
