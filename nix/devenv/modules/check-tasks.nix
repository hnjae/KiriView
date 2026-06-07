# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  qtCxxqt,
  ...
}:
let
  repoRoot = lib.escapeShellArg config.devenv.root;
  hostRuntimeLibraryPath = lib.concatStringsSep ":" [
    "${config.devenv.root}/.devenv/profile/lib"
    (lib.makeLibraryPath [
      (lib.getLib pkgs.pipewire)
      (lib.getLib pkgs.stdenv.cc.cc)
    ])
  ];
  hostTaskPrelude = ''
    set -euo pipefail

    cd ${repoRoot}

    export LC_ALL=C.UTF-8
    export LANG=C.UTF-8

    ${qtCxxqt.runtimeEnvironment}
    unset QT_ADDITIONAL_PACKAGES_PREFIX_PATH

    host_runtime_library_path=${lib.escapeShellArg hostRuntimeLibraryPath}
    nix_runtime_library_path=""
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
            nix_runtime_library_path="''${nix_runtime_library_path:+$nix_runtime_library_path:}$library_path"
        fi
    done
    export LD_LIBRARY_PATH="$host_runtime_library_path''${nix_runtime_library_path:+:$nix_runtime_library_path}''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  '';
  testJobsPrelude = ''
    test_jobs="''${KIRIVIEW_TEST_JOBS:-$(nproc)}"
    if ! [[ $test_jobs =~ ^[0-9]+$ ]] || ((test_jobs < 1)); then
        printf 'Invalid KIRIVIEW_TEST_JOBS value: %s\n' "$test_jobs" >&2
        exit 2
    fi
  '';
  lintJobsPrelude = ''
    lint_jobs="''${KIRIVIEW_LINT_JOBS:-$(nproc)}"
    if ! [[ $lint_jobs =~ ^[0-9]+$ ]] || ((lint_jobs < 1)); then
        printf 'Invalid KIRIVIEW_LINT_JOBS value: %s\n' "$lint_jobs" >&2
        exit 2
    fi
  '';
in
{
  scripts."run-clazy-parallel" = {
    description = "Run clazy-standalone in parallel with ordered output";
    packages = [ pkgs.coreutils ];
    exec = ''
      set -euo pipefail

      usage() {
          cat >&2 <<'EOF'
      Usage: run-clazy-parallel [options] -- <source>...

      Options:
        --jobs <count>          Number of clazy-standalone jobs to run in parallel.
        --clazy-binary <path>   clazy-standalone binary to execute.
        --checks <checks>       clazy checks to run.
        --ignore-dirs <regex>   Directories for clazy to ignore.
        -p, --compile-db <dir>  Compilation database directory.
      EOF
      }

      jobs="''${CLAZY_JOBS:-}"
      clazy_binary="''${CLAZY_BINARY:-clazy-standalone}"
      checks="''${CLAZY_CHECKS:-level0}"
      ignore_dirs=""
      compile_db="."

      while (($# > 0)); do
          case "$1" in
          --jobs)
              if (($# < 2)); then
                  usage
                  exit 2
              fi
              jobs="$2"
              shift 2
              ;;
          --jobs=*)
              jobs="''${1#--jobs=}"
              shift
              ;;
          --clazy-binary)
              if (($# < 2)); then
                  usage
                  exit 2
              fi
              clazy_binary="$2"
              shift 2
              ;;
          --clazy-binary=*)
              clazy_binary="''${1#--clazy-binary=}"
              shift
              ;;
          --checks)
              if (($# < 2)); then
                  usage
                  exit 2
              fi
              checks="$2"
              shift 2
              ;;
          --checks=*)
              checks="''${1#--checks=}"
              shift
              ;;
          --ignore-dirs)
              if (($# < 2)); then
                  usage
                  exit 2
              fi
              ignore_dirs="$2"
              shift 2
              ;;
          --ignore-dirs=*)
              ignore_dirs="''${1#--ignore-dirs=}"
              shift
              ;;
          -p | --compile-db)
              if (($# < 2)); then
                  usage
                  exit 2
              fi
              compile_db="$2"
              shift 2
              ;;
          --compile-db=*)
              compile_db="''${1#--compile-db=}"
              shift
              ;;
          --help)
              usage
              exit 0
              ;;
          --)
              shift
              break
              ;;
          *)
              usage
              exit 2
              ;;
          esac
      done

      if [[ -z $jobs ]]; then
          jobs="$(nproc)"
      fi
      if ! [[ $jobs =~ ^[0-9]+$ ]] || ((jobs < 1)); then
          printf 'Invalid --jobs value: %s\n' "$jobs" >&2
          exit 2
      fi

      sources=("$@")
      source_count="$#"
      if ((source_count == 0)); then
          exit 0
      fi

      tmp_dir="$(mktemp -d)"
      trap 'rm -rf "$tmp_dir"' EXIT

      run_clazy_for_source() {
          local index="$1"
          local source="$2"
          local output_file="$tmp_dir/$index.out"
          local status_file="$tmp_dir/$index.status"
          local status

          set +e
          {
              printf '[%d/%d] Processing file %s.\n' "$((index + 1))" "$source_count" "$source"
              if [[ -n $ignore_dirs ]]; then
                  "$clazy_binary" \
                      --checks="$checks" \
                      --ignore-dirs="$ignore_dirs" \
                      -p "$compile_db" \
                      "$source"
              else
                  "$clazy_binary" \
                      --checks="$checks" \
                      -p "$compile_db" \
                      "$source"
              fi
          } >"$output_file" 2>&1
          status="$?"
          printf '%s\n' "$status" >"$status_file"
          exit 0
      }

      printf 'Running clazy-standalone for %d files with %d jobs...\n' "$source_count" "$jobs"

      running_jobs=0
      for ((index = 0; index < source_count; index++)); do
          run_clazy_for_source "$index" "''${sources[$index]}" &
          running_jobs=$((running_jobs + 1))
          if ((running_jobs >= jobs)); then
              wait -n
              running_jobs=$((running_jobs - 1))
          fi
      done

      while ((running_jobs > 0)); do
          wait -n
          running_jobs=$((running_jobs - 1))
      done

      exit_status=0
      for ((index = 0; index < source_count; index++)); do
          output_file="$tmp_dir/$index.out"
          status_file="$tmp_dir/$index.status"

          if [[ -f $output_file ]]; then
              cat "$output_file"
          fi

          if [[ ! -f $status_file ]]; then
              printf 'clazy-standalone did not report a status for %s.\n' "''${sources[$index]}" >&2
              exit_status=1
              continue
          fi

          read -r source_status <"$status_file"
          if ((source_status != 0)); then
              exit_status=1
          fi
      done

      exit "$exit_status"
    '';
  };

  tasks = {
    "ci:test:rust" = {
      description = "Run host Rust library and doc tests";
      exec = ''
        ${hostTaskPrelude}
        ${testJobsPrelude}

        printf 'Running host Rust tests with nextest using %d jobs...\n' "$test_jobs"
        CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target"} \
            kiriview-rust-host-env \
            cargo \
                nextest \
                run \
                --locked \
                --lib \
                --all-features \
                --build-jobs "$test_jobs" \
                --test-threads "$test_jobs"

        printf 'Running host Rust doc tests with %d jobs...\n' "$test_jobs"
        CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target"} \
            kiriview-rust-host-env \
            cargo \
                test \
                --doc \
                --locked \
                --all-features \
                --jobs "$test_jobs" \
                -- \
                --test-threads "$test_jobs"
      '';
    };

    "ci:test:cpp" = {
      description = "Run host C++ tests against the Cargo-owned KiriView app library";
      after = [
        "ci:test:rust@succeeded"
      ];
      exec = ''
        ${hostTaskPrelude}
        ${testJobsPrelude}

        printf 'Building Cargo-owned KiriView app library artifacts with %d jobs...\n' "$test_jobs"
        CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target"} \
            kiriview-rust-host-env \
            cargo \
                build \
                --locked \
                --lib \
                --all-features \
                --jobs "$test_jobs"

        cmake \
            -S tests/cpp \
            -B target/devenv/cpp-tests \
            -DCMAKE_BUILD_TYPE=Debug \
            -DKIRIVIEW_CARGO_TARGET_DIR=${lib.escapeShellArg "${config.devenv.root}/target/debug"}
        printf 'Building and running host C++ tests with %d jobs...\n' "$test_jobs"
        cmake --build target/devenv/cpp-tests --parallel "$test_jobs"
        # GNU gettext ignores LANGUAGE under C/POSIX locales; devenv defaults to C.UTF-8.
        LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 \
            ctest \
                --test-dir target/devenv/cpp-tests \
                --output-on-failure \
                --parallel "$test_jobs"
      '';
    };

    "ci:lint:rust" = {
      description = "Run Rust clippy";
      after = [
        "ci:test:cpp@succeeded"
      ];
      exec = ''
        ${hostTaskPrelude}
        unset LD_LIBRARY_PATH
        unset QT_PLUGIN_PATH
        ${lintJobsPrelude}

        kiriview-rust-host-env \
            cargo \
                clippy \
                --locked \
                --all-targets \
                --all-features \
                --jobs "$lint_jobs" \
            -- \
            -D warnings \
            2>&1 \
            | cat
      '';
    };

    "ci:lint:qml" = {
      description = "Run qmllint against QML sources";
      exec = ''
        set -euo pipefail

        cd ${repoRoot}

        export LC_ALL=C.UTF-8
        export LANG=C.UTF-8
        unset LD_LIBRARY_PATH
        unset QT_PLUGIN_PATH
        unset QT_ADDITIONAL_PACKAGES_PREFIX_PATH

        ${lib.getExe' pkgs.kdePackages.qtdeclarative "qmllint"} ${qtCxxqt.qmlLintImportArgs} --ignore-settings --max-warnings 0 src/qml/*.qml \
            2>&1 \
            | sed \
                -e '/^Two plugins named "Quick" present, make sure no plugins are duplicated\. The second plugin will not be loaded\.$/d' \
                -e '/^Two plugins named "QtDesignStudio" present, make sure no plugins are duplicated\. The second plugin will not be loaded\.$/d'
      '';
    };

    "ci:lint:cpp" = {
      description = "Run clang-tidy and clazy against C++ sources";
      after = [
        "ci:lint:rust@succeeded"
      ];
      exec = ''
        ${qtCxxqt.cppLintPrelude}

        ${hostTaskPrelude}
        unset QT_PLUGIN_PATH
        ${lintJobsPrelude}

        ${lib.getExe' pkgs.llvmPackages.clang-unwrapped "run-clang-tidy"} \
            -clang-tidy-binary ${lib.getExe' pkgs.clang-tools "clang-tidy"} \
            -header-filter=${lib.escapeShellArg "^${config.devenv.root}/src/"} \
            -exclude-header-filter=${lib.escapeShellArg "^${config.devenv.root}/(target|build-dir)/"} \
            -p . \
            -j "$lint_jobs" \
            -quiet \
            ${qtCxxqt.cppSourcesShellArgs}

        run-clazy-parallel \
            --jobs "$lint_jobs" \
            --clazy-binary ${lib.getExe' pkgs.clazy "clazy-standalone"} \
            --checks "''${CLAZY_CHECKS:-level0}" \
            --ignore-dirs=${lib.escapeShellArg qtCxxqt.clazyIgnoreDirsRegex} \
            -p . \
            -- \
            ${qtCxxqt.cppSourcesShellArgs}
      '';
    };
  };
}
