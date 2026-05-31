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

| Situation                                                                                                       | What to run                                                                                                                                           |
| --------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Iterating on code                                                                                               | Smallest relevant focused checks: targeted unit/integration tests, `qmllint` on touched QML, clippy/check on touched Rust crates, scoped C++/Qt lint. |
| Intent-only commit (spec / architecture / test-only)                                                            | Stay narrow; skip the full suite unless the change is unusually risky.                                                                                |
| Finishing a code change                                                                                         | `just test` (default, unless the user asks to skip/pause).                                                                                            |
| Touched lint/i18n, QML structure, generated catalogs/templates, formatting rules, or cross-language integration | `just check`.                                                                                                                                         |
| Touched Flatpak packaging, runtime deps, or manifest test behavior                                              | `just build-with-tests`.                                                                                                                              |

Always report any skipped focused or final check in your final response.

## Commands

- `devenv shell` — development environment.
- `just build` / `just build-with-tests` — Flatpak build in `build-dir/` (tests off / on).
- `just run` — launch from `build-dir/`.
- `devenv shell -- treefmt` / `treefmt --ci` — format / CI format check.
- Lint all languages: `devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:lint`
- Host tests: `devenv tasks run --refresh-eval-cache --refresh-task-cache --mode before kiriview:test:host`

## Targeted test recipes

All recipes assume the devenv lld prefix: `export RUSTFLAGS="${RUSTFLAGS:+$RUSTFLAGS }-C link-arg=-fuse-ld=lld"`.

**Rust (prefer a filtered lib test before the full suite)** — `<filter>` e.g. `imagezoomstate`:

```sh
devenv shell -- bash -lc 'export RUSTFLAGS="${RUSTFLAGS:+$RUSTFLAGS }-C link-arg=-fuse-ld=lld"; CARGO_TARGET_DIR=target cargo test --locked --lib --all-features <filter> -- --test-threads "$(nproc)"'
```

**C++/Qt (build host Rust artifacts, then run the matching CTest target)** — `<test_target>` e.g. `test_imagezoomstate`:

```sh
devenv shell -- bash -lc 'export RUSTFLAGS="${RUSTFLAGS:+$RUSTFLAGS }-C link-arg=-fuse-ld=lld"; CARGO_TARGET_DIR=target cargo build --locked --lib --all-features'
devenv shell -- cmake -S tests/cpp -B target/devenv/cpp-tests -DCMAKE_BUILD_TYPE=Debug -DKIRIVIEW_CARGO_TARGET_DIR="$PWD/target/debug"
devenv shell -- cmake --build target/devenv/cpp-tests --target <test_target> --parallel "$(nproc)"
devenv shell -- env LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 ctest --test-dir target/devenv/cpp-tests -R '^<test_target>$' --output-on-failure
```
