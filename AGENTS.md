# Repository Guidelines

## Project Overview

KiriView is a desktop app using Rust, CXX-Qt and KDE Kirigami. Key paths: `src/` app code, `tests/cpp/` C++ tests, `po/` translations, `flatpak/` packaging patches, and `nix/` devenv support.

## Architecture Guidance

For language-boundary, module ownership, FFI design, workflow structure, and other long-term maintainability decisions, follow `docs/architecture/ARCHITECTURE.md`. Keep architecture rules in that document or ADRs instead of duplicating them here.

## Compatibility Policy

KiriView is still pre-release. Do not spend effort preserving backward compatibility or writing migrations for existing user data, configuration, APIs, or internal formats unless explicitly requested.

## Spec-Driven Development

Product specifications live in `docs/spec/*.md`; there is no aggregate spec file. Follow spec-driven development for product behavior and user-facing implementation changes. Before implementing a product behavior change, update every relevant spec file under `docs/spec/` with the intended user-visible behavior, commit that spec update, then implement against the committed spec. Development-only or repository-maintenance changes do not require spec updates.

## Build, Test, and Development Commands

- `devenv shell`: development environment.
- `just build`: Flatpak build in `build-dir/`, tests disabled.
- `just build-with-tests`: full Flatpak build with manifest tests.
- `just run`: launch from `build-dir/`.
- `just lint`: host/devenv Rust, QML, and C++ linters.
- `just test`: fast host Rust library tests and host C++ subset.
- `just format`: run treefmt formatters.
- `just format-check`: check treefmt formatting in CI mode.

## Coding Style & Naming Conventions

Rust, QML, C++, and headers use 4-space indentation. Keep app IDs as `io.github.hnjae.KiriView`.

## Markdown Style

Write documentation in a concise, technical style. Use Mermaid for diagrams when diagrams are needed. Do not hard-wrap prose in Markdown documents. Keep ordinary paragraphs and list item text on a single source line unless Markdown syntax, tables, code blocks, or another structured block require line breaks.

## Translation Policy

Do not personally author translations for individual language files such as `po/*.po`. Only update translation templates or existing translation files when the strings are supplied by a translator, upstream tooling, or an explicit user request.

## Commit & Pull Request Guidelines

Use Conventional Commit style, such as `feat: init project`. After completing a requested repository change, create a commit before ending the task unless the user explicitly asks not to commit or asks to pause. If the work naturally requires multiple independent steps, commit after each step is complete, keeping the relevant `docs/spec/*.md` update as the first commit for product behavior changes. Check `git status` before committing and stage paths explicitly. Do not include unrelated user changes in these commits.

## Licensing & Configuration

This repository is AGPL-3.0-or-later and uses REUSE checks. New source files should include SPDX copyright and license headers; generated or metadata files should be covered in `REUSE.toml` when inline headers are not practical.
