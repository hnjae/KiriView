# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
let
  rustHostLldBin = pkgs.runCommandLocal "kiriview-rust-host-lld-bin" { } ''
    mkdir -p "$out/bin"
    ln -s ${lib.getExe' pkgs.lld "ld.lld"} "$out/bin/ld"
    ln -s ${lib.getExe' pkgs.lld "ld.lld"} "$out/bin/ld.lld"
    ln -s ${lib.getExe' pkgs.lld "lld"} "$out/bin/lld"
  '';
  rustHostLinker = pkgs.writeShellApplication {
    name = "kiriview-rust-host-linker";
    text = ''
      exec ${lib.getExe' pkgs.stdenv.cc "cc"} -B${rustHostLldBin}/bin/ "$@"
    '';
  };
  rustHostCargoTargetDir = "${config.devenv.root}/target";
  rustHostLinkerFlag = "-C linker=${lib.getExe rustHostLinker}";
  rustHostEnvironment = # sh
    ''
      export CARGO_TARGET_DIR=${lib.escapeShellArg rustHostCargoTargetDir}

      case " ''${RUSTFLAGS:-} " in
      *" ${rustHostLinkerFlag} "*)
          ;;
      *)
          export RUSTFLAGS="''${RUSTFLAGS:+$RUSTFLAGS }${rustHostLinkerFlag}"
          ;;
      esac
    '';
in
{
  _module.args = {
    inherit
      rustHostCargoTargetDir
      rustHostEnvironment
      ;
  };
}
