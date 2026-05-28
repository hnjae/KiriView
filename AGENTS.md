# Repository Guidelines

## Project Overview

KiriView is a Rust, CXX-Qt, and KDE Kirigami desktop app. Key paths: `src/` app code, `tests/cpp/` C++ tests, `docs/` project documentation, `po/` translations, `flatpak/` packaging patches, and `nix/` devenv support.

## Architecture Guidance

For language boundaries, module ownership, FFI design, workflow structure, and other long-term maintainability decisions, follow `docs/architecture/`, starting from `docs/architecture/README.md`. Keep architecture rules in those documents or ADRs instead of duplicating them here.

## Compatibility Policy

KiriView is currently in pre-release. Backward compatibility for configurations, APIs, and internal formats should not be maintained unless explicitly requested.

## Spec-Driven Development

Product specifications live in the subject files under `docs/spec/`; `docs/spec/README.md` is an index, not an aggregate spec. Use spec-driven development for product behavior and user-facing implementation changes. Before implementing a product behavior change, update every relevant subject file under `docs/spec/` with the intended user-visible behavior, commit that spec update, then implement against the committed spec. Development-only or repository-maintenance changes do not require spec updates.

## Build, Test, and Development Commands

- `devenv shell`: development environment.
- `just build`: Flatpak build in `build-dir/`, tests disabled.
- `just build-with-tests`: full Flatpak build with manifest tests.
- `just run`: launch from `build-dir/`.
- `devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:lint`: host/devenv Rust, QML, and C++ linters.
- `devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:test:host`: host Rust library tests and host C++ subset.
- `devenv shell -- treefmt`: run treefmt formatters.
- `devenv shell -- treefmt --ci`: check treefmt formatting in CI mode.

### Agent Testing Workflow

During development, run the smallest relevant focused test set first. Before ending a code-change task, run `just test` unless the user explicitly asks to skip or pause. Use `just check` when lint/i18n/check wiring is affected. Use `just build-with-tests` when Flatpak packaging, runtime dependencies, or manifest test behavior is affected. Report any skipped focused or final test in the final response.

For Rust policy changes, prefer a filtered library test before the full suite:

```sh
devenv shell -- zsh -lc 'export RUSTFLAGS="${RUSTFLAGS:+$RUSTFLAGS }-C link-arg=-fuse-ld=lld"; CARGO_TARGET_DIR=target cargo test --locked --lib --all-features <filter> -- --test-threads "$(nproc)"'
```

For C++/Qt runtime changes, build the host Rust artifacts and run the matching CTest target:

```sh
devenv shell -- zsh -lc 'export RUSTFLAGS="${RUSTFLAGS:+$RUSTFLAGS }-C link-arg=-fuse-ld=lld"; CARGO_TARGET_DIR=target cargo build --locked --lib --all-features'
devenv shell -- cmake -S tests/cpp -B target/devenv/cpp-tests -DCMAKE_BUILD_TYPE=Debug -DKIRIVIEW_CARGO_TARGET_DIR="$PWD/target/debug"
devenv shell -- cmake --build target/devenv/cpp-tests --target <test_target> --parallel "$(nproc)"
devenv shell -- env LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 ctest --test-dir target/devenv/cpp-tests -R '^<test_target>$' --output-on-failure
```

Use `<filter>` for Rust test/module filters such as `imagezoomstate`, and `<test_target>` for CTest targets such as `test_imagezoomstate`.

## Coding Style & Naming Conventions

Rust, QML, C++, and headers use 4-space indentation. Keep app IDs as `io.github.hnjae.KiriView`.

## Markdown Style

- Write all documentation in a concise, technical style.
- Do not hard-wrap prose in Markdown files. Keep ordinary paragraphs and list items on a single source line unless Markdown syntax or structured blocks require line breaks.
- Use Mermaid for diagrams when diagrams are needed.

## Translation Policy

Do not personally author translations for individual language files such as `po/*.po`. Only update translation templates or existing translation files when the strings are supplied by a translator, upstream tooling, or an explicit user request.

## Commit & Pull Request Guidelines

Use Conventional Commit style, such as `feat: init project`. After a requested repository change, create a commit before ending the task unless the user explicitly asks not to commit or asks to pause. If the work naturally requires multiple independent steps, commit after each step is complete, keeping the relevant `docs/spec/` subject-file update as the first commit for product behavior changes. Check `git status` before committing and stage paths explicitly. Do not include unrelated user changes in these commits.

## Licensing & Configuration

This repository is AGPL-3.0-or-later and uses REUSE checks. New source files should include SPDX copyright and license headers; generated or metadata files should be covered in `REUSE.toml` when inline headers are impractical.
