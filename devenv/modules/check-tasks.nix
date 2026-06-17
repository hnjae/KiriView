# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  config,
  pkgs,
  lib,
  qtCxxqt,
  rustHostCargoTargetDir,
  rustHostEnvironment,
  ...
}:
let
  repoRoot = lib.escapeShellArg config.devenv.root;
  baseTaskPrelude = # sh
    ''
      set -euo pipefail

      cd ${repoRoot}

      export LC_ALL=C.UTF-8
      export LANG=C.UTF-8
    '';
  qtRuntimePrelude = # sh
    ''
      ${qtCxxqt.qtRuntimeEnvironment}
      unset QT_ADDITIONAL_PACKAGES_PREFIX_PATH
    '';
  qtBuildPrelude = # sh
    ''
      ${qtCxxqt.qtBuildEnvironment}
      unset QT_ADDITIONAL_PACKAGES_PREFIX_PATH
    '';
  testJobsPrelude = # sh
    ''
      test_jobs="''${KIRIVIEW_TEST_JOBS:-$(nproc)}"
      if ! [[ $test_jobs =~ ^[0-9]+$ ]] || ((test_jobs < 1)); then
          printf 'Invalid KIRIVIEW_TEST_JOBS value: %s\n' "$test_jobs" >&2
          exit 2
      fi
    '';
  lintJobsPrelude = # sh
    ''
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
    exec = # sh
      ''
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
    "dev:lsp:refresh" = {
      description = "Refresh generated LSP metadata for rust-analyzer and clangd";
      exec = # sh
        ''
          ${baseTaskPrelude}
          ${qtBuildPrelude}
          ${rustHostEnvironment}
          ${testJobsPrelude}

          printf 'Building Cargo-owned KiriView app library artifacts with %d jobs...\n' "$test_jobs"
          cargo \
              build \
              --locked \
              --lib \
              --all-features \
              --jobs "$test_jobs"

          cmake_make_program="$(command -v make)"
          cmake \
              -S tests/cpp \
              -B target/devenv/cpp-tests \
              -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_MAKE_PROGRAM="$cmake_make_program" \
              -DKIRIVIEW_CARGO_TARGET_DIR=${lib.escapeShellArg "${rustHostCargoTargetDir}/debug"}

          ${lib.getExe qtCxxqt.refreshCxxqtIncludes}
        '';
    };

    "ci:test:rust" = {
      description = "Run host Rust library and doc tests";
      exec = # sh
        ''
          ${baseTaskPrelude}
          ${qtRuntimePrelude}
          ${rustHostEnvironment}
          ${testJobsPrelude}

          printf 'Running host Rust tests with nextest using %d jobs...\n' "$test_jobs"
          cargo \
              nextest \
              run \
              --locked \
              --lib \
              --all-features \
              --build-jobs "$test_jobs" \
              --test-threads "$test_jobs"

          printf 'Running host Rust doc tests with %d jobs...\n' "$test_jobs"
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
      showOutput = true;
      exec = # sh
        ''
          ${baseTaskPrelude}
          ${qtRuntimePrelude}
          ${rustHostEnvironment}
          ${testJobsPrelude}

          printf 'Building Cargo-owned KiriView app library artifacts with %d jobs...\n' "$test_jobs"
          cargo \
              build \
              --locked \
              --lib \
              --all-features \
              --jobs "$test_jobs"

          cmake_make_program="$(command -v make)"
          cmake \
              -S tests/cpp \
              -B target/devenv/cpp-tests \
              -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_MAKE_PROGRAM="$cmake_make_program" \
              -DKIRIVIEW_CARGO_TARGET_DIR=${lib.escapeShellArg "${rustHostCargoTargetDir}/debug"}
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
      exec = # sh
        ''
          ${baseTaskPrelude}
          ${qtBuildPrelude}
          ${rustHostEnvironment}
          ${lintJobsPrelude}

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
      exec = # sh
        ''
          ${baseTaskPrelude}
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

    "ci:lint:flatpak" = {
      description = "Check Flatpak manifest permission policy";
      exec = # sh
        ''
          ${baseTaskPrelude}

          manifest="org.hnjae.kiriview.json"

          has_finish_arg() {
              ${lib.getExe pkgs.jq} \
                  --exit-status \
                  --arg arg "$1" \
                  '.["finish-args"] | arrays | index($arg) != null' \
                  "$manifest" >/dev/null
          }

          require_arg() {
              if ! has_finish_arg "$1"; then
                  printf 'Flatpak manifest must include permission: %s\n' "$1" >&2
                  return 1
              fi
          }

          forbid_arg() {
              if has_finish_arg "$1"; then
                  printf 'Flatpak manifest must not include broad permission: %s\n' "$1" >&2
                  return 1
              fi
          }

          require_arg "--nofilesystem=/run/user"
          require_arg "--filesystem=xdg-run/pipewire-0"
          require_arg "--filesystem=xdg-run/gvfs"
          require_arg "--filesystem=xdg-cache/thumbnails:create"
          require_arg "--talk-name=org.kde.KIOFuse"

          forbid_arg "--filesystem=/run/user"
          forbid_arg "--filesystem=/run/user:ro"
          forbid_arg "--filesystem=xdg-run"
          forbid_arg "--filesystem=xdg-cache"
          forbid_arg "--filesystem=xdg-cache:ro"
          forbid_arg "--filesystem=xdg-cache:create"
        '';
    };

    "ci:lint:desktop" = {
      description = "Validate desktop metadata";
      exec = # sh
        ''
          ${baseTaskPrelude}

          ${lib.getExe' pkgs.desktop-file-utils "desktop-file-validate"} org.hnjae.kiriview.desktop
        '';
    };

    "ci:lint:cpp" = {
      description = "Run clang-tidy and clazy against C++ sources";
      after = [
        "ci:lint:rust@succeeded"
      ];
      exec = # sh
        ''
          ${qtCxxqt.cppLintPrelude}

          ${baseTaskPrelude}
          ${qtBuildPrelude}
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
