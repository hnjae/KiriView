# Evolution Rules

When adding or moving logic:

1. Start from ownership: policy in Rust, Qt/KDE execution in C++.
1. Keep FFI value-based and explicit.
1. Move whole policy decisions, not isolated boolean branches.
1. Keep each workflow value under one canonical owner.
1. Keep QObject/QML API changes contained inside C++ facade classes.
1. Avoid adding compatibility layers for internal formats unless explicitly requested.
1. Document meaningful architecture decisions in the relevant architecture document or in an ADR.
