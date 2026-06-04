# Repository Guidelines

KiriView is a Rust + CXX-Qt + KDE Kirigami desktop app.
Layout: `src/` (app), `tests/cpp/` (C++ tests), `docs/` (docs), `po/` (translations), `flatpak/` (packaging), `nix/` (devenv).

## Policies

- **Architecture lives in `docs/architecture/`** (start at `README.md`). Follow it for language boundaries, module ownership, FFI, and workflow structure. Record long-term rules there or in ADRs — never duplicate them here.
- **Pre-release: no backward compatibility.** Do not preserve compatibility for configs, APIs, or internal formats unless explicitly asked.

## Project conventions

- 4-space indentation for Rust, QML, C++, and headers.
- App ID is always `io.github.hnjae.KiriView`.
- **Translations:** never author `po/*.po` content yourself. Only update templates or existing files with strings supplied by a translator, upstream tooling, or an explicit request.
- **Licensing:** AGPL-3.0-or-later with REUSE checks. New source files need SPDX copyright + license headers; cover generated/metadata files in `REUSE.toml` when inline headers are impractical.

## Verification discipline

**Scope verification to the change's blast radius: stay narrow while iterating, go wide before finishing.**

| Situation                                                                                                   | What to run                                                                                                                                           |
| ----------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Iterating on code                                                                                           | Smallest relevant focused checks: targeted unit/integration tests, `qmllint` on touched QML, clippy/check on touched Rust crates, scoped C++/Qt lint. |
| Intent-only commit (spec / architecture / test-only)                                                        | Stay narrow; skip the full suite unless the change is unusually risky.                                                                                |
| Finishing a code change                                                                                     | `devenv tasks run ci:test` (default, unless the user asks to skip/pause).                                                                             |
| Touched QML files or QML structure                                                                          | `devenv tasks run ci:lint:qml` plus the relevant Qt/QML CTest target; use `devenv tasks run ci:test` if no focused target covers the behavior.        |
| Touched i18n source strings, generated catalog templates, or generated catalogs                             | `devenv tasks run ci:i18n:pot-check`; also run `devenv tasks run ci:i18n:check` when `po/*.po` files changed.                                         |
| Touched formatter config or formatting rules                                                                | `devenv shell -- treefmt --ci` plus the relevant hook or lint task for the changed file type.                                                         |
| Touched lint rules or lint task wiring                                                                      | Run the affected lint task; use `devenv tasks run ci:lint` when the affected lint surface is broad.                                                   |
| Touched cross-language integration, native source manifests, check-task orchestration, or dependency wiring | `devenv tasks run ci`.                                                                                                                                |
| Touched Flatpak packaging, runtime deps, or manifest test behavior                                          | `just build-with-tests`.                                                                                                                              |

Always report any skipped focused or final check in your final response.

## Commands

- `devenv shell` — development environment.
- `just build` / `just build-with-tests` — Flatpak build in `build-dir/` (tests off / on).
- `just run` — launch from `build-dir/`.
- `devenv shell -- treefmt` / `treefmt --ci` — format / CI format check.
- Lint all languages: `devenv tasks run ci:lint`
- Tests: `devenv tasks run ci:test`

## Targeted test recipes

All Rust host recipes run through `kiriview-rust-host-env`, which owns the host linker configuration.

**Rust (prefer a filtered lib test before the full suite)** — `<filter>` e.g. `imagezoomstate`:

```sh
devenv shell -- env CARGO_TARGET_DIR=target kiriview-rust-host-env cargo test --locked --lib --all-features <filter> -- --test-threads "$(nproc)"
```

**C++/Qt (build host Rust artifacts, then run the matching CTest target)** — `<test_target>` e.g. `test_imagezoomstate`:

```sh
devenv shell -- env CARGO_TARGET_DIR=target kiriview-rust-host-env cargo build --locked --lib --all-features
devenv shell -- cmake -S tests/cpp -B target/devenv/cpp-tests -DCMAKE_BUILD_TYPE=Debug -DKIRIVIEW_CARGO_TARGET_DIR="$PWD/target/debug"
devenv shell -- cmake --build target/devenv/cpp-tests --target <test_target> --parallel "$(nproc)"
devenv shell -- env LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 ctest --test-dir target/devenv/cpp-tests -R '^<test_target>$' --output-on-failure
```
