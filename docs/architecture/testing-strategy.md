# Testing Strategy

Test Rust policy in Rust unit tests when the logic is independent of Qt. These tests should cover state transitions, edge cases, and policy tables.

Test C++ runtime code with Qt tests when behavior depends on:

- Qt object lifetime or signals.
- `QImage`, `QUrl`, or rendering data.
- KIO or file-operation adapters.
- Controller integration across async boundaries.

Do not duplicate every Rust policy test in C++. C++ tests should verify that the runtime layer applies plans correctly and preserves integration behavior.
