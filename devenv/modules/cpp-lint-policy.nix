# SPDX-FileCopyrightText: 2026 KIM Hyunjae
# SPDX-License-Identifier: AGPL-3.0-or-later
{
  env.CLAZY_CHECKS = builtins.concatStringsSep "," [
    # Checks from Manual Level:
    # "assert-with-side-effects" # managed by clang-tidy: bugprone-assert-side-effect
    # "unneeded-cast"            # managed by clang-tidy: readability-redundant-casting/cppcoreguidelines-pro-type-*
    # "unused-result-check"      # managed by clang-tidy: bugprone-unused-return-value
    "ifndef-define-typo"
    "qhash-with-char-pointer-key"
    "qproperty-type-mismatch"
    "qstring-varargs"
    "signal-with-return-value"
    "unexpected-flag-enumerator-value"
    "thread-with-slots"
    "tr-non-literal"
    "raw-environment-function"
    "qvariant-template-instantiation"
    "container-inside-loop"
    "detaching-member"
    "isempty-vs-count"
    "reserve-candidates"
    "use-chrono-in-qtimer"
    "use-arrow-operator-instead-of-data"

    # Checks from Level 0-1:
    "level1"
    # Requires manual ownership decisions for QObject/moc integration and copy semantics.
    # clazy fixits previously added Q_OBJECT/Q_DISABLE_COPY without matching build metadata.
    # "no-rule-of-two-soft" # managed by clang-tidy: cppcoreguidelines-special-member-functions

    # Checks from Level 2:
    # "function-args-by-ref"      # managed by clang-tidy: performance-unnecessary-value-param
    # "implicit-casts"            # managed by clang-tidy: bugprone-bool-pointer-implicit-conversion
    # "returning-void-expression" # managed by clang-tidy: readability-avoid-return-with-void-value
    # "rule-of-three"             # managed by clang-tidy: cppcoreguidelines-special-member-functions
    # "virtual-call-ctor"         # managed by clang-tidy: clang-analyzer-cplusplus.PureVirtualCall/clang-analyzer-optin.cplusplus.VirtualCall
    "base-class-event"
    # "copyable-polymorphic" # requires manual copy/move and ABI ownership decisions
    "ctor-missing-parent-argument"
    "function-args-by-value"
    # "missing-qobject-macro" # requires adding matching moc build metadata when accepted
    "old-style-connect"
    "qstring-allocations"
    "static-pmf"
  ];

  env.CLAZY_FIXIT = builtins.concatStringsSep "," [
    # Checks from Manual Level:
    "fix-qbytearray-conversion-to-c-style"
    "fix-missing-qstringref"
    "fix-qt-keyword-emit"
    "fix-qt-keywords"
    "fix-sanitize-inline-keyword"

    # Checks from Level 0:
    "fix-fully-qualified-moc-types"
    "fix-qcolor-from-literal"
    "fix-qdatetime-utc"
    "fix-qfileinfo-exists"
    "fix-qgetenv"

    # Checks from Level 1:
    "fix-auto-unexpected-qstringbuilder"
    "fix-range-loop-add-qasconst"
    "fix-range-loop-add-ref"

    # Checks from Level 2:
    # "fix-function-args-by-ref" # check managed by clang-tidy
    # "fix-missing-qobject-macro" # Q_OBJECT additions require matching moc build metadata
    "fix-old-style-connect"
  ];
}
