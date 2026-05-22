// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::imageinputclassification::extension_for_file_name;

struct VideoFormat {
    extensions: &'static [&'static str],
}

const SUPPORTED_DIRECT_VIDEO_FORMATS: &[VideoFormat] = &[
    VideoFormat {
        extensions: &["mp4"],
    },
    VideoFormat {
        extensions: &["m4v"],
    },
    VideoFormat {
        extensions: &["mov"],
    },
];

pub(crate) fn supported_direct_video_extensions() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_DIRECT_VIDEO_FORMATS
            .iter()
            .flat_map(|format| format.extensions.iter().copied()),
    )
}

pub(crate) fn is_supported_direct_video_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| direct_video_extension_is_supported(extension.as_str()))
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
