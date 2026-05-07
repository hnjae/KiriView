# Repository Guidelines

## Project Structure & Module Organization

KiriView is a Rust 2024 desktop app using CXX-Qt and KDE Kirigami.

## Spec-Driven Development

Follow spec-driven development for product behavior and user-facing implementation
changes. Before implementing a product behavior change, update `SPEC.md` to
describe the intended behavior from the user's perspective, commit that
specification update, and then carry out the work according to the committed spec.
Do not update `SPEC.md` for implementation-dependent changes that do not affect
user-visible behavior. This includes development-only or verification-only
changes, such as build tooling, formatter/linter/test configuration, CI wiring,
dependency metadata, or repository maintenance.

## Build, Test, and Development Commands

- `devenv shell`: enter the development environment with Rust, Qt, Flatpak, and formatters.
- `just build`: build the Flatpak app into `build-dir/` with tests disabled.
- `just build-with-tests`: run the full Flatpak build plus manifest test commands.
- `just run`: launch `kiriview` from the Flatpak build directory.
- `just lint`: run host/devenv Rust and C++ linters using the normal host Cargo target directory.
- `just test`: run fast host/devenv Rust library tests and the host C++ test subset.
- `just test-flatpak`: run the full Flatpak test path with offline `cargo test --all-targets --all-features` and all C++ tests.
- `just format`: run the configured pre-commit formatters and checks for all files.
- `just format-check`: verify Rust formatting with `cargo fmt --all --check`.

## Coding Style & Naming Conventions

Rust, QML, C++, and headers use 4-space indentation. Keep app IDs as `io.github.hnjae.KiriView`.

## Commit & Pull Request Guidelines

Use Conventional Commit style, such as `feat: init project`. After completing a
requested repository change, create a commit before ending the task unless the
user explicitly asks not to commit or asks to pause. If the work naturally
requires multiple independent steps, commit after each step is complete,
keeping the `SPEC.md` update as the first commit for product behavior changes.
Do not include unrelated user changes in these commits. Pull requests should
describe the change, list the commands run, link related issues when applicable,
and include screenshots or notes for visible QML/UI changes.

## Licensing & Configuration

This repository is AGPL-3.0-or-later and uses REUSE checks. New source files should include SPDX copyright and license headers; generated or metadata files should be covered in `REUSE.toml` when inline headers are not practical.
