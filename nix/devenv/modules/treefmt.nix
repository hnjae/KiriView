# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
{
  treefmt = {
    enable = true;

    config = {
      programs = {
        just.enable = true;
        nixfmt.enable = true;
        rustfmt = {
          enable = true;
          package = config.languages.rust.toolchain.rustfmt;
        };
        taplo.enable = true;
        yamlfmt.enable = true;
      };

      settings.formatter = {
        biome = {
          command = lib.getExe pkgs.biome;
          options = [
            "format"
            "--write"
            "--no-errors-on-unmatched"
          ];
          includes = [
            "*.js"
            "*.ts"
            "*.mjs"
            "*.mts"
            "*.cjs"
            "*.cts"
            "*.jsx"
            "*.tsx"
            "*.d.ts"
            "*.d.cts"
            "*.d.mts"
            "*.json"
            "*.jsonc"
            "*.css"
          ];
        };

        clang-format = {
          command = lib.getExe' pkgs.clang-tools "clang-format";
          options = [
            "-i"
            "--style=file"
          ];
          includes = [
            "*.c"
            "*.cc"
            "*.cpp"
            "*.cxx"
            "*.h"
            "*.hh"
            "*.hpp"
            "*.hxx"
          ];
        };

        qmlformat = {
          command = lib.getExe (
            pkgs.runCommandLocal "qmlformat" { } ''
              mkdir -p "$out/bin"
              ln -s "${lib.getExe' pkgs.kdePackages.qtdeclarative "qmlformat"}" "$out/bin/qmlformat"
            ''
          );
          options = [ "-i" ];
          includes = [ "*.qml" ];
        };

        shell-format = {
          command = lib.getExe (
            pkgs.writeShellApplication {
              name = "treefmt-shell-format";
              runtimeInputs = with pkgs; [
                shellharden
                shfmt
              ];
              text = ''
                for file in "$@"; do
                  shellharden --replace "$file"
                  shfmt --indent 4 --simplify --write "$file"
                done
              '';
            }
          );
          includes = [
            "*.sh"
            "*.bash"
            ".envrc"
            ".envrc.*"
            ".env"
            ".env.*"
          ];
        };
      };
    };
  };
}
