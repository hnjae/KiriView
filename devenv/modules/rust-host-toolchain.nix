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
      filtered_args=()
      for arg in "$@"; do
          case "$arg" in
              -fuse-ld=gold)
                  ;;
              *)
                  filtered_args+=("$arg")
                  ;;
          esac
      done

      exec ${lib.getExe' pkgs.stdenv.cc "cc"} -B${rustHostLldBin}/bin/ "''${filtered_args[@]}"
    '';
  };
  rustHostCargoTargetDir = "${config.devenv.root}/target";
  rustHostLinkerFlag = "-C linker=${lib.getExe rustHostLinker}";
  localJobsPrelude = # sh
    ''
      kiriview_default_local_jobs() {
          local cpu_count
          cpu_count="$(nproc)"
          printf '%d\n' "$((cpu_count / 2 + 1))"
      }

      if [[ -n ''${KIRIVIEW_JOBS:-} ]]; then
          kiriview_jobs="$KIRIVIEW_JOBS"
          kiriview_jobs_source="KIRIVIEW_JOBS"
      else
          kiriview_jobs="$(kiriview_default_local_jobs)"
          kiriview_jobs_source="local default job count"
      fi

      if ! [[ $kiriview_jobs =~ ^[0-9]+$ ]] || ((kiriview_jobs < 1)); then
          printf 'Invalid %s value: %s\n' "$kiriview_jobs_source" "$kiriview_jobs" >&2
          exit 2
      fi
    '';
  rustHostEnvironment = # sh
    ''
      ${localJobsPrelude}

      export CARGO_TARGET_DIR=${lib.escapeShellArg rustHostCargoTargetDir}
      export CARGO_BUILD_JOBS="''${CARGO_BUILD_JOBS:-$kiriview_jobs}"
      export RUST_TEST_THREADS="''${RUST_TEST_THREADS:-$kiriview_jobs}"

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
