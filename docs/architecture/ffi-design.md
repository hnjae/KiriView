# FFI Design

FFI code should be intentionally boring. A good bridge is explicit, typed, and easy to audit.

Use small bridge structs and enums for:

- Stable policy inputs.
- State snapshots.
- Change sets.
- Effect plans.

Avoid bridges that expose:

- Raw Qt object ownership.
- Long-lived Rust references to C++ objects.
- Implicit side effects hidden behind policy functions.
- One-off wrappers whose only purpose is to move a local C++ `switch` into Rust.

When a Rust module starts to look like glue, either move the branch back to C++ or absorb it into a larger Rust workflow reducer where it becomes part of a coherent policy decision.
