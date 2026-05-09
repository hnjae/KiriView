# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  ...
}:
let
  qtCxxqt = import ../internal/qt-cxxqt.nix { inherit config pkgs lib; };
  repoRoot = lib.escapeShellArg config.devenv.root;
  hostTaskPrelude = ''
    set -euo pipefail

    cd ${repoRoot}
  '';
in
{
  tasks = {
    "kiriview:test:rust-host" = {
      description = "Run host Rust library tests";
      exec = ''
        ${hostTaskPrelude}

        CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target"} \
            cargo test --locked --lib --all-features
        CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target"} \
            cargo build --locked --lib --all-features
      '';
    };

    "kiriview:test:cpp-host" = {
      description = "Run host C++ tests against host Rust artifacts";
      after = [ "kiriview:test:rust-host" ];
      exec = ''
        ${hostTaskPrelude}

        cmake \
            -S tests/cpp \
            -B target/devenv/cpp-tests \
            -DCMAKE_BUILD_TYPE=Debug \
            -DKIRIVIEW_CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target/debug"}
        cmake --build target/devenv/cpp-tests
        # GNU gettext ignores LANGUAGE under C/POSIX locales; devenv defaults to C.UTF-8.
        LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 \
            ctest \
                --test-dir target/devenv/cpp-tests \
                --output-on-failure \
                -E '^test_kiriimagedecoder$'
      '';
    };

    "kiriview:test:host" = {
      description = "Run host Rust and C++ tests";
      after = [ "kiriview:test:cpp-host" ];
      exec = "true";
    };

    "kiriview:lint:clippy" = {
      description = "Run Rust clippy";
      exec = ''
        ${hostTaskPrelude}

        cargo clippy --locked --all-targets --all-features -- -D warnings
      '';
    };

    "kiriview:lint:qmllint" = {
      description = "Run qmllint against QML sources";
      after = [ "kiriview:lint:clippy" ];
      exec = ''
        ${hostTaskPrelude}

        ${lib.getExe' pkgs.kdePackages.qtdeclarative "qmllint"} ${qtCxxqt.qmlLintImportArgs} --ignore-settings --max-warnings 0 src/qml/*.qml
      '';
    };

    "kiriview:lint:cpp" = {
      description = "Run C++ linters";
      after = [ "kiriview:lint:clippy" ];
      exec = ''
        ${qtCxxqt.cppLintPrelude}
        ${lib.getExe' pkgs.clang-tools "clang-tidy"} --quiet -p . ${qtCxxqt.cppSourcesShellArgs}
        ${lib.getExe' pkgs.clazy "clazy-standalone"} --checks="''${CLAZY_CHECKS:-level0}" --ignore-dirs=${lib.escapeShellArg qtCxxqt.clazyIgnoreDirsRegex} -p . ${qtCxxqt.cppSourcesShellArgs}
      '';
    };

    "kiriview:lint" = {
      description = "Run linters";
      after = [
        "kiriview:lint:qmllint"
        "kiriview:lint:cpp"
      ];
      exec = "true";
    };

    "kiriview:check" = {
      description = "Run KiriView host checks";
      after = [
        "kiriview:lint"
        "kiriview:test:host"
      ];
      before = [ "devenv:enterTest" ];
      exec = "true";
    };
  };
}
