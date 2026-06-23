# Repository Guidelines

KiriView is a Rust + CXX-Qt + KDE Kirigami desktop app.
Layout: `src/` (app), `tests/cpp/` (C++ tests), `docs/` (docs), `po/` (translations), `flatpak/` (packaging), `nix/` (devenv).

## Project conventions

- 4-space indentation for Rust, QML, C++, and headers.
- App ID is always `org.hnjae.kiriview`.
- **Translations:** never author `po/*.po` content yourself. Only update templates or existing files with strings supplied by a translator, upstream tooling, or an explicit request.
- **Licensing:** AGPL-3.0-or-later with REUSE checks. New source files need SPDX copyright + license headers; cover generated/metadata files in `REUSE.toml` when inline headers are impractical.

## Policies

- **Documentation routing:** `docs/spec/` is for user-visible behavior only, including visible unsupported states and disabled controls; do not put implementation details, milestone labels, test strategy, or coverage IDs there. `docs/architecture/` (start at `README.md`) is for durable design boundaries and ownership rules, including provider, render, state, language-boundary, FFI, workflow, and architecture test ownership; record long-term rules there or in ADRs, never duplicate them here, and do not put milestone acceptance criteria or deferred-work lists there. `docs/planning/` is for milestone scope, deferred work, non-goals, and acceptance criteria; planning does not override current behavior in `docs/spec/` or durable boundaries in `docs/architecture/`.
- **Pre-release: no backward compatibility.** Do not preserve compatibility for configs, APIs, or internal formats unless explicitly asked.

## External dependency and upstream boundaries

- Do not add, replace, vendor, or patch external libraries unless the user explicitly asks for that change.
- Do not modify upstream, third-party, generated, vendored, or packaged dependency sources as a workaround unless explicitly requested.
- If the correct or straightforward fix appears to require adding an external dependency, changing dependency versions, patching upstream code, or carrying a local workaround for an upstream or library defect, stop before implementing and ask the user how they want to proceed.
- When asking, briefly present the tradeoff, including the dependency or upstream option and any local workaround option.
- Do not avoid this pause by implementing a convoluted local workaround when a dependency addition or upstream patch is the clean solution.

## Test ownership

Tests must stay within the ownership boundary of the code under test. Do not assert unrelated repository artifacts such as `justfile`, Flatpak manifests, CI wiring, packaging metadata, or maintenance scripts from runtime/component tests. If those artifacts need policy coverage, add a dedicated lint or check task at the owning layer instead.

## Verification discipline

**Scope verification to the change's blast radius: stay narrow while iterating, go wide before finishing.**

| Situation                                                                                                   | What to run                                                                                                                                                                                                                                                                                 |
| ----------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Iterating on code                                                                                           | Smallest relevant focused checks: targeted unit/integration tests, `devenv tasks run --mode single ci:lint:qml` for touched QML, clippy/check on touched Rust crates, scoped C++/Qt lint.                                                                                                   |
| Intent-only commit (spec / architecture / test-only)                                                        | Stay narrow; skip the full suite unless the change is unusually risky.                                                                                                                                                                                                                      |
| Finishing a code change                                                                                     | `devenv tasks run --mode single ci:test` (default, unless the user asks to skip/pause).                                                                                                                                                                                                     |
| Touched Rust source, Rust tests, or Rust-side FFI behavior                                                  | Prefer a filtered host Rust test while iterating; use `devenv tasks run ci:test:rust` for the full host Rust test task and `devenv tasks run --mode single ci:lint:rust` for lint-only checks.                                                                                              |
| Touched C++/Qt source, C++ tests, CXX-Qt consumers, or native presentation/render/navigation behavior       | Prefer a focused CTest target while iterating; use `devenv tasks run --mode single ci:test:cpp` for the full host C++ test task and `devenv tasks run --mode single ci:lint:cpp` for lint-only checks. Run `just build-with-tests` when Flatpak-only decoder or codec behavior is in scope. |
| Touched QML files or QML structure                                                                          | `devenv tasks run --mode single ci:lint:qml` plus the relevant Qt/QML CTest target; use `devenv tasks run --mode single ci:test` if no focused target covers the behavior.                                                                                                                  |
| Touched i18n source strings, generated catalog templates, or generated catalogs                             | `devenv tasks run --mode single ci:i18n:pot-check`; also run `devenv tasks run --mode single ci:i18n:check` when `po/*.po` files changed.                                                                                                                                                   |
| Touched formatter config or formatting rules                                                                | `devenv shell -- treefmt --ci` plus the relevant hook or lint task for the changed file type.                                                                                                                                                                                               |
| Touched lint rules or lint task wiring                                                                      | Run the affected lint task with `--mode single` when you want lint-only execution; use `devenv tasks run --mode single ci:lint` when the affected lint surface is broad.                                                                                                                    |
| Touched cross-language integration, native source manifests, check-task orchestration, or dependency wiring | `devenv tasks run ci`.                                                                                                                                                                                                                                                                      |
| Touched Flatpak packaging, runtime deps, or manifest test behavior                                          | `just build-with-tests`.                                                                                                                                                                                                                                                                    |

Always report any skipped focused or final check in your final response.

`devenv tasks run` accepts prefix groups: `ci:test`, `ci:lint`, and `ci` run the matching `ci:*` tasks even though those exact aggregate names may not appear in `devenv tasks list`. Add `--mode single` when you want only the matched task group without unrelated upstream dependencies.

## Commands

- `devenv shell` — development environment.
- `just build` / `just build-with-tests` — Flatpak build in `build-dir/` (tests off / on).
- `just run` — launch from `build-dir/`.
- `devenv shell -- treefmt` / `devenv shell -- treefmt --ci` — format / CI format check.
- Lint all languages only: `devenv tasks run --mode single ci:lint`
- Lint Rust / C++ / QML only: `devenv tasks run --mode single ci:lint:rust`, `devenv tasks run --mode single ci:lint:cpp`, `devenv tasks run --mode single ci:lint:qml`
- Tests: `devenv tasks run --mode single ci:test`
- Test Rust / C++ only: `devenv tasks run --mode single ci:test:rust`, `devenv tasks run --mode single ci:test:cpp`

## Targeted test recipes

**Rust (prefer a filtered lib test before the full suite)** — `<filter>` e.g. `imagezoomstate`:

```sh
devenv shell -- env CARGO_TARGET_DIR=target cargo nextest run --locked --lib --all-features --build-jobs "$(nproc)" --test-threads "$(nproc)" <filter>
```

**C++/Qt (build the Cargo-owned app staticlib and generated headers, then run the matching CTest target)** — `<test_target>` e.g. `test_imagezoomstate`:

For the full host C++ test task, run `devenv tasks run --mode single ci:test:cpp` when you intentionally want only the C++ task; it skips declared Rust test dependencies and builds the required Cargo-owned KiriView app library artifacts inside the task. Without `--mode single`, `ci:test:cpp` runs `ci:test:rust` first. The CMake build compiles C++ test binaries that link that Cargo-produced app library.

```sh
devenv shell -- env CARGO_TARGET_DIR=target cargo build --locked --lib --all-features
devenv shell -- cmake -S tests/cpp -B target/devenv/cpp-tests -DCMAKE_BUILD_TYPE=Debug -DKIRIVIEW_CARGO_TARGET_DIR="$PWD/target/debug"
devenv shell -- cmake --build target/devenv/cpp-tests --target <test_target> --parallel "$(nproc)"
devenv shell -- env LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 ctest --test-dir target/devenv/cpp-tests -R '^<test_target>$' --output-on-failure
```
