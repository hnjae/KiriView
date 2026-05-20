# Architecture Overview

KiriView is a KDE Kirigami image viewer built from three cooperating layers:

```mermaid
flowchart TD
    QML["QML / Kirigami"]
    Facade["C++ QObject and QQuickItem facade"]
    Runtime["C++ Qt/KDE runtime and effect executor"]
    Rust["Rust Qt-independent policy core"]

    QML --> Facade --> Runtime --> Rust
    Rust -- "typed plans, state deltas, and effect descriptions" --> Runtime
```

The main maintenance goal is to keep product policy testable without making Rust own Qt runtime concerns. Rust defines policy decisions. C++ executes them through Qt and KDE.

The public QML facade layer is grouped in `src/facade/`. Domain runtime code remains in directories such as `src/document/`, `src/presentation/`, `src/rendering/`, `src/navigation/`, and `src/application/`; Rust policy bridge files remain under `src/policy/`.
