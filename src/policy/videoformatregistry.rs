// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::imageinputclassification::extension_for_file_name;

struct VideoFormat {
    extensions: &'static [&'static str],
    mime_types: &'static [&'static str],
}

const SUPPORTED_DIRECT_VIDEO_FORMATS: &[VideoFormat] = &[
    VideoFormat {
        extensions: &["mp4"],
        mime_types: &["video/mp4"],
    },
    VideoFormat {
        extensions: &["m4v"],
        mime_types: &["video/x-m4v"],
    },
    VideoFormat {
        extensions: &["mov"],
        mime_types: &["video/quicktime"],
    },
];

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustSupportedDirectVideoExtensions"]
        fn rust_supported_direct_video_extensions() -> Vec<String>;

        #[cxx_name = "rustSupportedDirectVideoMimeTypes"]
        fn rust_supported_direct_video_mime_types() -> Vec<String>;

        #[cxx_name = "rustIsSupportedDirectVideoFileName"]
        fn rust_is_supported_direct_video_file_name(name: &str) -> bool;

        #[cxx_name = "rustIsSupportedDirectVideoMimeTypeName"]
        fn rust_is_supported_direct_video_mime_type_name(mime_type_name: &str) -> bool;
    }
}

fn rust_supported_direct_video_extensions() -> Vec<String> {
    supported_direct_video_extensions()
}

fn rust_supported_direct_video_mime_types() -> Vec<String> {
    supported_direct_video_mime_types()
}

fn rust_is_supported_direct_video_file_name(name: &str) -> bool {
    is_supported_direct_video_file_name(name)
}

fn rust_is_supported_direct_video_mime_type_name(mime_type_name: &str) -> bool {
    is_supported_direct_video_mime_type_name(mime_type_name)
}

pub(crate) fn supported_direct_video_extensions() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_DIRECT_VIDEO_FORMATS
            .iter()
            .flat_map(|format| format.extensions.iter().copied()),
    )
}

pub(crate) fn supported_direct_video_mime_types() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_DIRECT_VIDEO_FORMATS
            .iter()
            .flat_map(|format| format.mime_types.iter().copied()),
    )
}

pub(crate) fn is_supported_direct_video_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| direct_video_extension_is_supported(extension.as_str()))
}

pub(crate) fn is_supported_direct_video_mime_type_name(mime_type_name: &str) -> bool {
    SUPPORTED_DIRECT_VIDEO_FORMATS
        .iter()
        .any(|format| format.mime_types.contains(&mime_type_name))
}

fn direct_video_extension_is_supported(extension: &str) -> bool {
    SUPPORTED_DIRECT_VIDEO_FORMATS
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
    fn supported_direct_video_extensions_are_sorted_and_unique() {
        assert_eq!(supported_direct_video_extensions(), ["m4v", "mov", "mp4"]);
    }

    #[test]
    fn supported_direct_video_mime_types_are_sorted_and_unique() {
        assert_eq!(
            supported_direct_video_mime_types(),
            ["video/mp4", "video/quicktime", "video/x-m4v"]
        );
    }

    #[test]
    fn matches_supported_video_file_names_case_insensitively() {
        assert!(is_supported_direct_video_file_name("clip.mp4"));
        assert!(is_supported_direct_video_file_name("clip.M4V"));
        assert!(is_supported_direct_video_file_name("clip.Mov"));
    }

    #[test]
    fn excludes_non_video_archive_and_directory_names() {
        for name in [
            "photo.png",
            "book.cbz",
            "archive.zip",
            "archive.tar",
            "archive.7z",
            "archive.rar",
            "directory",
            ".mp4",
            "clip.",
        ] {
            assert!(!is_supported_direct_video_file_name(name), "{name}");
        }
    }

    #[test]
    fn matches_supported_video_mime_types() {
        assert!(is_supported_direct_video_mime_type_name("video/mp4"));
        assert!(is_supported_direct_video_mime_type_name("video/x-m4v"));
        assert!(is_supported_direct_video_mime_type_name("video/quicktime"));
        assert!(!is_supported_direct_video_mime_type_name("application/zip"));
        assert!(!is_supported_direct_video_mime_type_name("image/png"));
    }

    #[test]
    fn direct_archive_entry_urls_remain_filename_eligible() {
        assert!(is_supported_direct_video_file_name(
            "zip:///path/archive.zip!/chapter/clip.MP4"
        ));
        assert!(is_supported_direct_video_file_name(
            "tar:///path/archive.tar!/clip.mov"
        ));
        assert!(!is_supported_direct_video_file_name(
            "zip:///path/archive.zip!/chapter/"
        ));
    }
}
