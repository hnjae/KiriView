# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
# Shared Qt/CXX-Qt tooling context consumed by devenv modules.
{
  config,
  pkgs,
  lib,
  karchivePackage ? pkgs.kdePackages.karchive,
}:
let
  qtToolPrefix = pkgs.symlinkJoin {
    name = "kiriview-qt-tools";
    paths = [
      karchivePackage
    ]
    ++ (with pkgs.kdePackages; [
      kconfig
      kimageformats
      ki18n
      kjobwidgets
      kirigami-addons
      kservice
      qtbase
      qtdeclarative
      qtimageformats
      qtmultimedia
      (qtmultimedia.dev or qtmultimedia)
      qtsvg
    ]);
  };
  qmake = pkgs.lib.hiPrio (
    pkgs.writeShellScriptBin "qmake6" ''
      if [ "$#" -eq 2 ] && [ "$1" = "-query" ]; then
        case "$2" in
          QT_INSTALL_PREFIX|QT_INSTALL_PREFIX/get|QT_HOST_PREFIX|QT_HOST_PREFIX/get|QT_INSTALL_ARCHDATA|QT_INSTALL_ARCHDATA/get|QT_INSTALL_DATA|QT_INSTALL_DATA/get|QT_HOST_DATA|QT_HOST_DATA/get)
            printf '%s\n' '${qtToolPrefix}'
            exit 0
            ;;
          QT_INSTALL_HEADERS|QT_INSTALL_HEADERS/get)
            printf '%s\n' '${qtToolPrefix}/include'
            exit 0
            ;;
          QT_INSTALL_LIBS|QT_INSTALL_LIBS/get|QT_HOST_LIBS|QT_HOST_LIBS/get)
            printf '%s\n' '${qtToolPrefix}/lib'
            exit 0
            ;;
          QT_HOST_LIBEXECS|QT_HOST_LIBEXECS/get|QT_INSTALL_LIBEXECS|QT_INSTALL_LIBEXECS/get)
            printf '%s\n' '${qtToolPrefix}/libexec'
            exit 0
            ;;
          QT_HOST_BINS|QT_HOST_BINS/get|QT_INSTALL_BINS|QT_INSTALL_BINS/get)
            printf '%s\n' '${qtToolPrefix}/bin'
            exit 0
            ;;
          QT_INSTALL_PLUGINS|QT_INSTALL_PLUGINS/get)
            printf '%s\n' '${qtToolPrefix}/lib/qt-6/plugins'
            exit 0
            ;;
          QT_INSTALL_QML|QT_INSTALL_QML/get)
            printf '%s\n' '${qtToolPrefix}/lib/qt-6/qml'
            exit 0
            ;;
        esac
      fi

      exec ${lib.getExe' pkgs.kdePackages.qtbase "qmake6"} "$@"
    ''
  );
  appQmlRoot = "${config.devenv.root}/target/cxxqt/qml_modules";
  qtQmlRoot = "${pkgs.kdePackages.qtdeclarative}/lib/qt-6/qml";
  qtmultimediaQmlRoot = "${pkgs.kdePackages.qtmultimedia}/lib/qt-6/qml";
  kconfigQmlRoot = "${pkgs.kdePackages.kconfig}/lib/qt-6/qml";
  ki18nQmlRoot = "${pkgs.kdePackages.ki18n}/lib/qt-6/qml";
  kirigamiQmlRoot = "${pkgs.kdePackages.kirigami.unwrapped}/lib/qt-6/qml";
  kirigamiAddonsQmlRoot = "${pkgs.kdePackages.kirigami-addons}/lib/qt-6/qml";
  qmlImportPaths = [
    appQmlRoot
    qtQmlRoot
    qtmultimediaQmlRoot
    kconfigQmlRoot
    ki18nQmlRoot
    kirigamiQmlRoot
    kirigamiAddonsQmlRoot
  ];
  hostRuntimeLibraryPath = lib.concatStringsSep ":" [
    "${config.devenv.root}/.devenv/profile/lib"
    (lib.makeLibraryPath [
      (lib.getLib pkgs.pipewire)
      (lib.getLib pkgs.stdenv.cc.cc)
    ])
  ];
  qtPluginPath = "${config.devenv.root}/.devenv/profile/lib/qt-6/plugins";
  readSourceManifest =
    path:
    lib.filter (source: source != "" && !(lib.hasPrefix "#" source)) (
      lib.splitString "\n" (builtins.readFile path)
    );
  cppCoreSources = readSourceManifest ../../src/cpp_core_sources.txt;
  cxxQtCppSources = readSourceManifest ../../src/cpp_cxxqt_sources.txt;
  cppSources = lib.sort builtins.lessThan (cxxQtCppSources ++ cppCoreSources);
  cppSourcesShellArgs = lib.escapeShellArgs cppSources;
  clazyIgnoreDirsRegex = "(^|/)(\\.devenv|target)(/|$)|^/nix/store/";
  qmlLintImportArgs = lib.escapeShellArgs (
    lib.concatMap (path: [
      "-I"
      path
    ]) qmlImportPaths
  );
  rustAnalyzerToml = # toml
    ''
      linkedProjects = ["Cargo.toml"]

      [cargo]
      targetDir = "target/rust-analyzer"

      [files]
      exclude = [
          ".cargo-vendor",
          ".devenv",
          ".direnv",
          ".flatpak-builder",
          ".flatpak-cargo",
          ".rumdl_cache",
          "build-dir",
          "repo",
          "target",
      ]
    '';
  compileCommandRecorder = pkgs.writeShellApplication {
    name = "kiriview-compile-command-recorder";
    runtimeInputs = with pkgs; [
      coreutils
      jq
      util-linux
    ];
    text = ''
      set -euo pipefail

      if (($# < 1)); then
          printf 'Usage: kiriview-compile-command-recorder <compiler> [args...]\n' >&2
          exit 2
      fi

      compiler="$1"
      shift

      compiler_path="$(command -v -- "$compiler" || true)"
      if [[ -z $compiler_path ]]; then
          printf 'Compiler not found: %s\n' "$compiler" >&2
          exit 127
      fi

      log_file="''${KIRIVIEW_COMPILE_COMMAND_LOG:-}"
      if [[ -n $log_file ]]; then
          has_compile_flag=0
          source_file=""
          for arg in "$@"; do
              if [[ $arg == "-c" ]]; then
                  has_compile_flag=1
              fi

              case "$arg" in
                  *.c | *.cc | *.cpp | *.cxx | *.C | *.CC | *.CPP | *.CXX)
                      source_file="$arg"
                      ;;
              esac
          done

          if ((has_compile_flag)) && [[ -n $source_file && ''${source_file##*/} != flag_check.cpp ]]; then
              mkdir -p "$(dirname "$log_file")"
              prefix_arguments_json="''${KIRIVIEW_COMPILE_COMMAND_PREFIX_ARGUMENTS_JSON:-}"
              jq_prefix_args=(--argjson prefix '[]')
              if [[ -n $prefix_arguments_json && -f $prefix_arguments_json ]]; then
                  jq_prefix_args=(--slurpfile prefix "$prefix_arguments_json")
              fi
              entry="$(
                  jq \
                      --null-input \
                      --compact-output \
                      "''${jq_prefix_args[@]}" \
                      --arg directory "$PWD" \
                      --arg file "$source_file" \
                      '($prefix[0] // []) as $prefix_arguments | $ARGS.positional as $arguments | { directory: $directory, file: $file, arguments: ([$arguments[0]] + $prefix_arguments + ($arguments[1:] // [])) }' \
                      --args -- "$compiler_path" "$@"
              )"
              lock_file="''${KIRIVIEW_COMPILE_COMMAND_LOG_LOCK:-$log_file.lock}"
              {
                  flock 9
                  printf '%s\n' "$entry" >>"$log_file"
              } 9>"$lock_file"
          fi
      fi

      exec "$compiler_path" "$@"
    '';
  };
  refreshCompileCommands = pkgs.writeShellApplication {
    name = "kiriview-refresh-compile-commands";
    runtimeInputs =
      with pkgs;
      [
        cmake
        coreutils
        gnumake
        jq
      ]
      ++ [ compileCommandRecorder ];
    text = ''
      set -euo pipefail

      if (($# != 2)); then
          printf 'Usage: kiriview-refresh-compile-commands <jobs> <cargo-debug-artifact-dir>\n' >&2
          exit 2
      fi

      jobs="$1"
      cargo_debug_artifact_dir="$2"

      if ! [[ $jobs =~ ^[0-9]+$ ]] || ((jobs < 1)); then
          printf 'Invalid job count: %s\n' "$jobs" >&2
          exit 2
      fi

      repo_root=${lib.escapeShellArg config.devenv.root}
      cd "$repo_root"

      metadata_dir="$repo_root/target/devenv"
      app_compile_log="$metadata_dir/app-compile-commands.jsonl"
      cxx_prefix_arguments_json="$metadata_dir/cxx-wrapper-arguments.json"
      tmp_cxx_prefix_arguments_json="$metadata_dir/cxx-wrapper-arguments.json.tmp.$$"
      cpp_tests_dir="$metadata_dir/cpp-tests"
      merged_compile_db="$metadata_dir/compile_commands.json"
      tmp_compile_db="$metadata_dir/compile_commands.json.tmp.$$"
      root_link_tmp="$repo_root/compile_commands.json.tmp.$$"

      mkdir -p "$metadata_dir"
      rm -f "$app_compile_log" "$app_compile_log.lock" "$tmp_cxx_prefix_arguments_json" "$tmp_compile_db" "$root_link_tmp"

      cxx_prefix_arguments=()
      while IFS= read -r include_dir; do
          cxx_prefix_arguments+=("-isystem" "$include_dir")
      done < <(
          c++ -E -x c++ - -v </dev/null 2>&1 \
              | sed -n '/#include <...> search starts here:/,/End of search list./p' \
              | sed '1d;$d;s/^ //;s/ (framework directory)$//'
      )
      jq \
          --null-input \
          '$ARGS.positional' \
          --args -- "''${cxx_prefix_arguments[@]}" \
          >"$tmp_cxx_prefix_arguments_json"
      mv -f "$tmp_cxx_prefix_arguments_json" "$cxx_prefix_arguments_json"

      printf 'Cleaning Cargo-owned KiriView app artifacts...\n'
      cargo clean -p kiriview

      printf 'Building Cargo-owned KiriView app library artifacts with %d jobs...\n' "$jobs"
      CC_KNOWN_WRAPPER_CUSTOM=kiriview-compile-command-recorder \
      CC="kiriview-compile-command-recorder cc" \
      CXX="kiriview-compile-command-recorder c++" \
      KIRIVIEW_COMPILE_COMMAND_LOG="$app_compile_log" \
      KIRIVIEW_COMPILE_COMMAND_PREFIX_ARGUMENTS_JSON="$cxx_prefix_arguments_json" \
          cargo \
              build \
              --locked \
              --lib \
              --all-features \
              --jobs "$jobs"

      if [[ ! -s $app_compile_log ]]; then
          printf 'Cargo app build did not capture any C++ compile commands.\n' >&2
          exit 1
      fi

      cmake_make_program="$(command -v make)"
      cmake \
          -S tests/cpp \
          -B "$cpp_tests_dir" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DCMAKE_MAKE_PROGRAM="$cmake_make_program" \
          -DKIRIVIEW_CARGO_TARGET_DIR="$cargo_debug_artifact_dir"

      if [[ ! -f $cpp_tests_dir/compile_commands.json ]]; then
          printf 'CMake did not emit %s.\n' "$cpp_tests_dir/compile_commands.json" >&2
          exit 1
      fi

      jq \
          --slurp \
          --arg repo_root "$repo_root" \
          '
              def normalize_file:
                  if (.file | type == "string" and startswith($repo_root + "/")) then
                      .file = (.file | .[($repo_root | length) + 1:])
                  else
                      .
                  end;

              map(if type == "array" then .[] else . end)
              | map(normalize_file)
              | sort_by(.file, .directory, (.arguments // [ .command ]))
          ' \
          "$app_compile_log" \
          "$cpp_tests_dir/compile_commands.json" \
          >"$tmp_compile_db"
      mv -f "$tmp_compile_db" "$merged_compile_db"

      ln -sT "target/devenv/compile_commands.json" "$root_link_tmp"
      mv -Tf "$root_link_tmp" "$repo_root/compile_commands.json"
    '';
  };
  refreshCxxqtIncludes = pkgs.writeShellApplication {
    name = "refresh-cxxqt-includes";
    runtimeInputs = with pkgs; [
      coreutils
      findutils
      util-linux
    ];
    text = ''
      set -euo pipefail

      repo_root=${lib.escapeShellArg config.devenv.root}
      cxxqt_clangd_include="$repo_root/target/cxxqt/clangd/include"
      lock_file="$repo_root/target/cxxqt/clangd/refresh.lock"

      mkdir -p "$cxxqt_clangd_include"
      exec 9>"$lock_file"
      flock 9

      find "$cxxqt_clangd_include" -mindepth 1 -maxdepth 1 -type l -delete

      link_generated_include_dir() {
          local generated_include_dir="$1"
          local generated_include
          local link_target

          if [[ ! -d "$generated_include_dir" ]]; then
              return
          fi

          while IFS= read -r -d "" generated_include; do
              link_target="$cxxqt_clangd_include/''${generated_include##*/}"
              rm -rf "$link_target"
              ln -sT "$generated_include" "$link_target"
          done < <(find "$generated_include_dir" -mindepth 1 -maxdepth 1 -print0)
      }

      while IFS= read -r generated_include_dir; do
          link_generated_include_dir "$generated_include_dir"
      done < <(
          find \
              "$repo_root"/target/rust-analyzer \
              "$repo_root"/target/devenv \
              "$repo_root"/target/flatpak \
              "$repo_root"/target/release \
              "$repo_root"/target/debug \
              \( -path "*/out/cxxqtbuild/include" -o -path "*/out/kconfig" \) \
              -type d \
              -printf "%T@ %p\n" 2>/dev/null \
              | sort -n \
              | sed "s/^[^ ]* //"
      )
    '';
  };
  cppLintPrelude = # sh
  ''
    set -euo pipefail

    cd ${lib.escapeShellArg config.devenv.root}
    ${lib.getExe refreshCxxqtIncludes}

    if [[ ! -f compile_commands.json ]]; then
        echo "compile_commands.json was not found; run 'devenv tasks run --mode single dev:lsp:refresh' to generate editor metadata" >&2
        exit 1
    fi
  ''
  +
    lib.optionalString (cppSources == [ ]) # sh
      ''
        echo "compile_commands.json does not contain any C++ sources" >&2
        exit 1
      '';
  qtBuildEnvironment = # sh
    ''
      export QMAKE=${lib.getExe' qmake "qmake6"}

      declare -A seen_cmake_prefix_paths=()
      cmake_prefix_path=""
      add_cmake_prefix_path() {
          local prefix_path=$1

          if [[ -z $prefix_path || ! -d $prefix_path || -v 'seen_cmake_prefix_paths[$prefix_path]' ]]; then
              return
          fi

          seen_cmake_prefix_paths[$prefix_path]=1
          cmake_prefix_path="''${cmake_prefix_path:+$cmake_prefix_path:}$prefix_path"
      }

      add_cmake_prefix_path ${lib.escapeShellArg "${config.devenv.root}/.devenv/profile"}
      IFS=: read -r -a existing_cmake_prefix_paths <<<"''${CMAKE_PREFIX_PATH:-}"
      for prefix_path in "''${existing_cmake_prefix_paths[@]}"; do
          add_cmake_prefix_path "$prefix_path"
      done
      export CMAKE_PREFIX_PATH="$cmake_prefix_path"
      unset seen_cmake_prefix_paths cmake_prefix_path prefix_path
      unset -f add_cmake_prefix_path
    '';
  qtRuntimeEnvironment = # sh
    ''
      ${qtBuildEnvironment}

      export QT_PLUGIN_PATH=${lib.escapeShellArg qtPluginPath}
      declare -A seen_runtime_library_paths=()
      runtime_library_path=""
      add_runtime_library_path() {
          local library_path=$1

          if [[ -z $library_path || ! -d $library_path || -v 'seen_runtime_library_paths[$library_path]' ]]; then
              return
          fi

          seen_runtime_library_paths[$library_path]=1
          runtime_library_path="''${runtime_library_path:+$runtime_library_path:}$library_path"
      }

      IFS=: read -r -a host_runtime_library_paths <<<${lib.escapeShellArg hostRuntimeLibraryPath}
      for library_path in "''${host_runtime_library_paths[@]}"; do
          add_runtime_library_path "$library_path"
      done

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

          add_runtime_library_path "$library_path"
      done

      IFS=: read -r -a existing_runtime_library_paths <<<"''${LD_LIBRARY_PATH:-}"
      for library_path in "''${existing_runtime_library_paths[@]}"; do
          add_runtime_library_path "$library_path"
      done
      export LD_LIBRARY_PATH="$runtime_library_path"
      unset seen_runtime_library_paths runtime_library_path library_path
      unset -f add_runtime_library_path
    '';
in
{
  inherit
    clazyIgnoreDirsRegex
    cppLintPrelude
    cppSourcesShellArgs
    qmake
    qmlLintImportArgs
    refreshCompileCommands
    refreshCxxqtIncludes
    rustAnalyzerToml
    qtBuildEnvironment
    qtRuntimeEnvironment
    ;

  enterShell = # sh
    ''
      ${qtRuntimeEnvironment}

      "${lib.getExe refreshCxxqtIncludes}"
    '';

  qmllsGeneral = {
    buildDir = "${config.devenv.root}/target/cxxqt/qml_modules";
    importPaths = lib.concatStringsSep ":" qmlImportPaths;
    "no-cmake-calls" = true;
  };

}
