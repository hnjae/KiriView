# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{ pkgs, lib, ... }:
{
  git-hooks.excludes = [
    "devenv\.lock"
  ];
  git-hooks.hooks = {
    # Static checkers
    detect-private-keys.enable = true;
    gitlint.enable = true;
    reuse.enable = true;
    typos.enable = true;
    desktop-file-utils-validate = {
      enable = true;
      name = "desktop-file-validate";
      files = ''\.desktop$'';
      entry = "${lib.getExe' pkgs.desktop-file-utils "desktop-file-validate"}";
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
