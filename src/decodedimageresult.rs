// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustFirstDisplayImageDecodeStatus {
        Ready = 0,
        NotImplemented = 1,
        Error = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustStaticDecodedImagePlan {
        UseFirstDisplay = 0,
        DecodeBlockingDisplay = 1,
        Fail = 2,
    }

    extern "Rust" {
        #[cxx_name = "rustStaticDecodedImagePlanAfterFirstDisplay"]
        fn rust_static_decoded_image_plan_after_first_display(
            status: RustFirstDisplayImageDecodeStatus,
            first_display_image_available: bool,
        ) -> RustStaticDecodedImagePlan;

        #[cxx_name = "rustStaticDecodedImagePlanAfterBlockingDisplay"]
        fn rust_static_decoded_image_plan_after_blocking_display(
            blocking_display_image_available: bool,
        ) -> RustStaticDecodedImagePlan;
    }
}

use ffi::{RustFirstDisplayImageDecodeStatus, RustStaticDecodedImagePlan};

fn rust_static_decoded_image_plan_after_first_display(
    status: RustFirstDisplayImageDecodeStatus,
    first_display_image_available: bool,
) -> RustStaticDecodedImagePlan {
    match status {
        RustFirstDisplayImageDecodeStatus::Ready => {
            if first_display_image_available {
                RustStaticDecodedImagePlan::UseFirstDisplay
            } else {
                RustStaticDecodedImagePlan::Fail
            }
        }
        RustFirstDisplayImageDecodeStatus::NotImplemented => {
            RustStaticDecodedImagePlan::DecodeBlockingDisplay
        }
        RustFirstDisplayImageDecodeStatus::Error => RustStaticDecodedImagePlan::Fail,
        _ => RustStaticDecodedImagePlan::Fail,
    }
}

fn rust_static_decoded_image_plan_after_blocking_display(
    blocking_display_image_available: bool,
) -> RustStaticDecodedImagePlan {
    if blocking_display_image_available {
        RustStaticDecodedImagePlan::DecodeBlockingDisplay
    } else {
        RustStaticDecodedImagePlan::Fail
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn first_display_ready_uses_available_image_only() {
        assert_eq!(
            rust_static_decoded_image_plan_after_first_display(
                RustFirstDisplayImageDecodeStatus::Ready,
                true,
            ),
            RustStaticDecodedImagePlan::UseFirstDisplay
        );
        assert_eq!(
            rust_static_decoded_image_plan_after_first_display(
                RustFirstDisplayImageDecodeStatus::Ready,
                false,
            ),
            RustStaticDecodedImagePlan::Fail
        );
    }

    #[test]
    fn first_display_not_implemented_falls_back_to_blocking_display() {
        assert_eq!(
            rust_static_decoded_image_plan_after_first_display(
                RustFirstDisplayImageDecodeStatus::NotImplemented,
                false,
            ),
            RustStaticDecodedImagePlan::DecodeBlockingDisplay
        );
    }

    #[test]
    fn first_display_error_fails_without_fallback() {
        assert_eq!(
            rust_static_decoded_image_plan_after_first_display(
                RustFirstDisplayImageDecodeStatus::Error,
                true,
            ),
            RustStaticDecodedImagePlan::Fail
        );
    }

    #[test]
    fn blocking_display_plan_requires_an_image() {
        assert_eq!(
            rust_static_decoded_image_plan_after_blocking_display(true),
            RustStaticDecodedImagePlan::DecodeBlockingDisplay
        );
        assert_eq!(
            rust_static_decoded_image_plan_after_blocking_display(false),
            RustStaticDecodedImagePlan::Fail
        );
    }
}
