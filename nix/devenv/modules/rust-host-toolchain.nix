# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
let
  hostRuntimeLibraryPath = lib.concatStringsSep ":" [
    "${config.devenv.root}/.devenv/profile/lib"
    (lib.makeLibraryPath [
      (lib.getLib pkgs.pipewire)
      (lib.getLib pkgs.stdenv.cc.cc)
    ])
  ];
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
  rustHostEnv = pkgs.writeShellApplication {
    name = "kiriview-rust-host-env";
    text = ''
      if (($# == 0)); then
          printf 'Usage: kiriview-rust-host-env <command> [args...]\n' >&2
          exit 2
      fi

      export RUSTFLAGS="''${RUSTFLAGS:+$RUSTFLAGS }-C linker=${lib.getExe rustHostLinker}"
      export PATH=${lib.escapeShellArg "${rustHostLldBin}/bin"}:"$PATH"
      runtime_library_path=${lib.escapeShellArg hostRuntimeLibraryPath}
      # shellcheck disable=SC2206
      nix_ldflags=( ''${NIX_LDFLAGS:-} )
      for ((i = 0; i < ''${#nix_ldflags[@]}; i++)); do
          case "''${nix_ldflags[$i]}" in
          -L)
              ((i += 1))
              library_path="''${nix_ldflags[$i]:-}"
              ;;
          -L*)
              library_path="''${nix_ldflags[$i]#-L}"
              ;;
          *)
              continue
              ;;
          esac

          if [[ -d $library_path ]]; then
              runtime_library_path="''${runtime_library_path:+$runtime_library_path:}$library_path"
          fi
      done
      export LD_LIBRARY_PATH="$runtime_library_path''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
      exec "$@"
    '';
  };
in
{
  packages = [
    rustHostLldBin
    rustHostLinker
    rustHostEnv
  ];
}
