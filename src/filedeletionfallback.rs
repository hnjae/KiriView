// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustDeletionFallbackDirection {
        Previous = 0,
        Next = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDeletionFallbackDirections {
        preferred: RustDeletionFallbackDirection,
        fallback: RustDeletionFallbackDirection,
    }

    extern "Rust" {
        #[cxx_name = "rustDeletionFallbackDirections"]
        fn rust_deletion_fallback_directions() -> RustDeletionFallbackDirections;
    }
}

use ffi::{RustDeletionFallbackDirection, RustDeletionFallbackDirections};

fn rust_deletion_fallback_directions() -> RustDeletionFallbackDirections {
    RustDeletionFallbackDirections {
        preferred: RustDeletionFallbackDirection::Next,
        fallback: RustDeletionFallbackDirection::Previous,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn deletion_fallback_prefers_next_then_previous() {
        let directions = rust_deletion_fallback_directions();

        assert_eq!(directions.preferred, RustDeletionFallbackDirection::Next);
        assert_eq!(directions.fallback, RustDeletionFallbackDirection::Previous);
    }
}
