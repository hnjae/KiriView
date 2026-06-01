# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  pkgs,
  lib,
  ...
}:
let
  lldAsLd = pkgs.runCommandLocal "kiriview-rust-host-lld-as-ld" { } ''
    mkdir -p "$out/bin"
    ln -s ${lib.getExe' pkgs.lld "ld.lld"} "$out/bin/ld"
  '';
  rustHostLinker = pkgs.writeShellApplication {
    name = "kiriview-rust-host-linker";
    text = ''
      exec ${lib.getExe' pkgs.stdenv.cc "cc"} -B${lldAsLd}/bin/ "$@"
    '';
  };
  rustHostEnv = pkgs.writeShellApplication {
    name = "kiriview-rust-host-env";
    text = ''
      if (($# == 0)); then
          printf 'Usage: kiriview-rust-host-env <command> [args...]\n' >&2
          exit 2
      fi

      export RUSTFLAGS="''${RUSTFLAGS:+$RUSTFLAGS }-C linker=${lib.getExe rustHostLinker}"
      exec "$@"
    '';
  };
in
{
  packages = [
    rustHostLinker
    rustHostEnv
  ];
}
