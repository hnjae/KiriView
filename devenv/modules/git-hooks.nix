# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  pkgs,
  lib,
  config,
  ...
}:
{
  git-hooks.excludes = [ ".*\\.lock$" ];

  git-hooks.hooks = {
    # Static checkers
    detect-private-keys.enable = true;
    cocogitto = {
      enable = true;
      name = "cog verify";
      description = "Lint commit messages with Cocogitto.";
      package = pkgs.cocogitto;
      entry = "${lib.getExe pkgs.cocogitto} verify --file";
      stages = [ "commit-msg" ];
    };
    reuse.enable = true;
    typos.enable = true;
    desktop-file-utils-validate = {
      enable = true;
      name = "desktop-file-validate";
      files = ''\.desktop$'';
      entry = "${lib.getExe' pkgs.desktop-file-utils "desktop-file-validate"}";
    };

    # Formatters
    treefmt.enable = true;

    # Nix
    deadnix.enable = true;
    statix.enable = true;

    # Miscellaneous Checkers
    rumdl.enable = true;
    shellcheck-env = {
      enable = true;
      name = "shellcheck";
      package = pkgs.shellcheck;
      files = ''
        (?x)^(
          .*\.(sh|bash)$|
          \.envrc(\..+)?$|
          \.env(\..+)?$
        )
      '';
      entry = lib.getExe pkgs.shellcheck;
    };
  };

  tasks."ci:git-hooks" = {
    exec = "${lib.getExe config.git-hooks.package} run --all-files";
  };
}
