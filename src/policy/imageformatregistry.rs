// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::archiveformat::supported_comic_book_archive_extensions;
use crate::imageinputclassification::{RAW_IMAGE_EXTENSIONS, extension_for_file_name};

struct ImageFormat {
    extensions: &'static [&'static str],
    mime_types: &'static [&'static str],
}

const RAW_IMAGE_MIME_TYPES: &[&str] = &[
    "image/x-adobe-dng",
    "image/x-canon-cr2",
    "image/x-canon-cr3",
    "image/x-canon-crw",
    "image/x-dcraw",
    "image/x-fuji-raf",
    "image/x-kde-raw",
    "image/x-kodak-dcr",
    "image/x-kodak-k25",
    "image/x-kodak-kdc",
    "image/x-minolta-mrw",
    "image/x-nikon-nef",
    "image/x-nikon-nrw",
    "image/x-olympus-orf",
    "image/x-panasonic-raw",
    "image/x-panasonic-raw2",
    "image/x-panasonic-rw",
    "image/x-panasonic-rw2",
    "image/x-pentax-pef",
    "image/x-sigma-x3f",
    "image/x-sony-arw",
    "image/x-sony-sr2",
    "image/x-sony-srf",
];

const SUPPORTED_IMAGE_FORMATS: &[ImageFormat] = &[
    ImageFormat {
        extensions: &["png"],
        mime_types: &["image/png", "image/apng"],
    },
    ImageFormat {
        extensions: &["jpeg", "jpg"],
        mime_types: &["image/jpeg"],
    },
    ImageFormat {
        extensions: &["jp2"],
        mime_types: &["image/jp2"],
    },
    ImageFormat {
        extensions: &["jxl"],
        mime_types: &["image/jxl"],
    },
    ImageFormat {
        extensions: &["gif"],
        mime_types: &["image/gif"],
    },
    ImageFormat {
        extensions: &["webp"],
        mime_types: &["image/webp"],
    },
    ImageFormat {
        extensions: &["avif"],
        mime_types: &["image/avif"],
    },
    ImageFormat {
        extensions: &["avifs"],
        mime_types: &["image/avif-sequence"],
    },
    ImageFormat {
        extensions: &["avci"],
        mime_types: &["image/avci"],
    },
    ImageFormat {
        extensions: &["heic", "heif", "hif"],
        mime_types: &["image/heic", "image/heif"],
    },
    ImageFormat {
        extensions: &["heics", "heifs"],
        mime_types: &["image/heic-sequence", "image/heif-sequence"],
    },
    ImageFormat {
        extensions: &["hej2"],
        mime_types: &["image/hej2k"],
    },
    ImageFormat {
        extensions: &["bmp"],
        mime_types: &["image/bmp"],
    },
    ImageFormat {
        extensions: &["tif", "tiff"],
        mime_types: &["image/tiff"],
    },
    ImageFormat {
        extensions: RAW_IMAGE_EXTENSIONS,
        mime_types: RAW_IMAGE_MIME_TYPES,
    },
    ImageFormat {
        extensions: &["svg"],
        mime_types: &["image/svg+xml"],
    },
];

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustSupportedImageExtensions"]
        fn rust_supported_image_extensions() -> Vec<String>;

        #[cxx_name = "rustSupportedImageMimeTypes"]
        fn rust_supported_image_mime_types() -> Vec<String>;

        #[cxx_name = "rustSupportedOpenExtensions"]
        fn rust_supported_open_extensions() -> Vec<String>;

        #[cxx_name = "rustIsSupportedImageFileName"]
        fn rust_is_supported_image_file_name(name: &str) -> bool;

        #[cxx_name = "rustIsSupportedRawImageFileName"]
        fn rust_is_supported_raw_image_file_name(name: &str) -> bool;
    }
}

fn rust_supported_image_extensions() -> Vec<String> {
    supported_image_extensions()
}

pub(crate) fn supported_image_extensions() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_IMAGE_FORMATS
            .iter()
            .flat_map(|format| format.extensions.iter().copied()),
    )
}

fn rust_supported_image_mime_types() -> Vec<String> {
    supported_image_mime_types()
}

pub(crate) fn supported_image_mime_types() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_IMAGE_FORMATS
            .iter()
            .flat_map(|format| format.mime_types.iter().copied()),
    )
}

fn rust_supported_open_extensions() -> Vec<String> {
    let mut extensions = rust_supported_image_extensions();
    extensions.extend(supported_comic_book_archive_extensions());
    extensions.sort();
    extensions
}

fn rust_is_supported_image_file_name(name: &str) -> bool {
    is_supported_image_file_name(name)
}

pub(crate) fn is_supported_image_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| image_extension_is_supported(extension.as_str()))
}

fn rust_is_supported_raw_image_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| RAW_IMAGE_EXTENSIONS.contains(&extension.as_str()))
}

fn image_extension_is_supported(extension: &str) -> bool {
    SUPPORTED_IMAGE_FORMATS
        .iter()
        .any(|format| format.extensions.contains(&extension))
}

fn unique_sorted_strings(values: impl IntoIterator<Item = &'static str>) -> Vec<String> {
    let mut strings: Vec<String> = values.into_iter().map(str::to_owned).collect();
    strings.sort();
    strings.dedup();
    strings
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn supported_open_extensions_include_images_and_comic_books_only() {
        let extensions = rust_supported_open_extensions();

        assert_eq!(extensions, {
            let mut sorted = extensions.clone();
            sorted.sort();
            sorted
        });
        assert!(extensions.contains(&"png".to_owned()));
        assert!(extensions.contains(&"cbz".to_owned()));
        assert!(!extensions.contains(&"zip".to_owned()));
        assert!(!extensions.contains(&"rar".to_owned()));
    }
}
