# FFI Design

FFI code should be explicit, typed, and audit-friendly.

Use small bridge structs and enums for:

- Stable policy inputs.
- State snapshots.
- Change sets.
- Effect plans.

Predecode policy inputs should describe source access profiles rather than concrete storage or collection kinds. C++ runtime code may translate a displayed source or opened collection scope into profile values such as neutral window size, biased-direction window size, and parallel limit, but Rust predecode scheduling should not branch on archive, directory, or backend implementation names.

Avoid bridges that expose:

- Raw Qt object ownership.
- Long-lived Rust references to C++ objects.
- Implicit side effects hidden behind policy functions.
- One-off wrappers whose only purpose is to move a local C++ `switch` into Rust.

When a Rust module starts to look like glue, either move the branch back to C++ or absorb it into a larger Rust workflow reducer where it becomes part of a coherent policy decision.

Rust bridge source files used by the CXX-Qt QML module live under `src/policy/` because CXX-Qt supports only one Rust source directory per QML module. Keep domain ownership visible through names, crate module declarations, tests, and architecture docs instead of scattering bridge files across C++ runtime directories. Every Rust policy source under `src/policy/` must be listed in `src/rust_policy_sources.txt`; CXX-Qt bridge files must also be listed in `src/rust_bridge_sources.txt` so bridge exposure remains an explicit subset of policy ownership.

C++ conversion helpers for Rust bridge values live under `src/bridge/`. These helpers should stay limited to value conversion between Qt/C++ types and generated Rust bridge types; they must not own runtime state, execute side effects, or hide policy decisions.
