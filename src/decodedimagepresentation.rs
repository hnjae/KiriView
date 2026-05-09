// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustDecodedImagePresentationKind {
        StaticImage = 0,
        DecodedAnimation = 1,
        ReaderAnimation = 2,
        HeifSequenceAnimation = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustDecodedImagePresentationTarget {
        StaticImage = 0,
        DecodedAnimation = 1,
        ReaderAnimation = 2,
        HeifSequenceAnimation = 3,
        DecodeError = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDecodedImagePresentationPlan {
        target: RustDecodedImagePresentationTarget,
        predecode_cacheable: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustDecodedImagePresentationPlan"]
        fn rust_decoded_image_presentation_plan(
            kind: RustDecodedImagePresentationKind,
            decoded_animation_frame_count: usize,
            static_image_cacheable: bool,
        ) -> RustDecodedImagePresentationPlan;
    }
}

use ffi::{
    RustDecodedImagePresentationKind, RustDecodedImagePresentationPlan,
    RustDecodedImagePresentationTarget,
};

fn rust_decoded_image_presentation_plan(
    kind: RustDecodedImagePresentationKind,
    decoded_animation_frame_count: usize,
    static_image_cacheable: bool,
) -> RustDecodedImagePresentationPlan {
    match kind {
        RustDecodedImagePresentationKind::StaticImage => presentation_plan(
            RustDecodedImagePresentationTarget::StaticImage,
            static_image_cacheable,
        ),
        RustDecodedImagePresentationKind::DecodedAnimation => {
            if decoded_animation_frame_count == 0 {
                presentation_plan(RustDecodedImagePresentationTarget::DecodeError, false)
            } else {
                presentation_plan(RustDecodedImagePresentationTarget::DecodedAnimation, false)
            }
        }
        RustDecodedImagePresentationKind::ReaderAnimation => {
            presentation_plan(RustDecodedImagePresentationTarget::ReaderAnimation, false)
        }
        RustDecodedImagePresentationKind::HeifSequenceAnimation => presentation_plan(
            RustDecodedImagePresentationTarget::HeifSequenceAnimation,
            false,
        ),
        _ => presentation_plan(RustDecodedImagePresentationTarget::DecodeError, false),
    }
}

fn presentation_plan(
    target: RustDecodedImagePresentationTarget,
    predecode_cacheable: bool,
) -> RustDecodedImagePresentationPlan {
    RustDecodedImagePresentationPlan {
        target,
        predecode_cacheable,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn static_images_preserve_cacheability_policy_result() {
        assert_eq!(
            rust_decoded_image_presentation_plan(
                RustDecodedImagePresentationKind::StaticImage,
                0,
                true,
            ),
            presentation_plan(RustDecodedImagePresentationTarget::StaticImage, true)
        );
        assert_eq!(
            rust_decoded_image_presentation_plan(
                RustDecodedImagePresentationKind::StaticImage,
                0,
                false,
            ),
            presentation_plan(RustDecodedImagePresentationTarget::StaticImage, false)
        );
    }

    #[test]
    fn decoded_animations_require_at_least_one_frame() {
        assert_eq!(
            rust_decoded_image_presentation_plan(
                RustDecodedImagePresentationKind::DecodedAnimation,
                1,
                true,
            ),
            presentation_plan(RustDecodedImagePresentationTarget::DecodedAnimation, false)
        );
        assert_eq!(
            rust_decoded_image_presentation_plan(
                RustDecodedImagePresentationKind::DecodedAnimation,
                0,
                true,
            ),
            presentation_plan(RustDecodedImagePresentationTarget::DecodeError, false)
        );
    }

    #[test]
    fn reader_and_heif_animations_are_presented_without_predecode_cacheability() {
        assert_eq!(
            rust_decoded_image_presentation_plan(
                RustDecodedImagePresentationKind::ReaderAnimation,
                0,
                true,
            ),
            presentation_plan(RustDecodedImagePresentationTarget::ReaderAnimation, false)
        );
        assert_eq!(
            rust_decoded_image_presentation_plan(
                RustDecodedImagePresentationKind::HeifSequenceAnimation,
                0,
                true,
            ),
            presentation_plan(
                RustDecodedImagePresentationTarget::HeifSequenceAnimation,
                false
            )
        );
    }
}
