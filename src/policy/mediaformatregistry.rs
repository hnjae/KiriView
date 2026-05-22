// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::imageformatregistry::{is_supported_image_file_name, supported_image_extensions};
use crate::videoformatregistry::{
    is_supported_direct_video_file_name, supported_direct_video_extensions,
};

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustSupportedOrdinaryMediaExtensions"]
        fn rust_supported_ordinary_media_extensions() -> Vec<String>;

        #[cxx_name = "rustIsSupportedOrdinaryMediaFileName"]
        fn rust_is_supported_ordinary_media_file_name(name: &str) -> bool;

        #[cxx_name = "rustIsSupportedDirectVideoFileName"]
        fn rust_is_supported_direct_video_file_name(name: &str) -> bool;
    }
}

fn rust_supported_ordinary_media_extensions() -> Vec<String> {
    supported_ordinary_media_extensions()
}

fn rust_is_supported_ordinary_media_file_name(name: &str) -> bool {
    is_supported_ordinary_media_file_name(name)
}

fn rust_is_supported_direct_video_file_name(name: &str) -> bool {
    is_supported_direct_video_file_name(name)
}

pub(crate) fn supported_ordinary_media_extensions() -> Vec<String> {
    let mut extensions = supported_image_extensions();
    extensions.extend(supported_direct_video_extensions());
    extensions.sort();
    extensions.dedup();
    extensions
}

pub(crate) fn is_supported_ordinary_media_file_name(name: &str) -> bool {
    is_supported_image_file_name(name) || is_supported_direct_video_file_name(name)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn supported_ordinary_media_extensions_are_sorted_and_unique() {
        let extensions = supported_ordinary_media_extensions();

        assert_eq!(extensions, {
            let mut sorted = extensions.clone();
            sorted.sort();
            sorted.dedup();
            sorted
        });
        assert!(extensions.contains(&"png".to_owned()));
        assert!(extensions.contains(&"mp4".to_owned()));
        assert!(extensions.contains(&"m4v".to_owned()));
        assert!(extensions.contains(&"mov".to_owned()));
    }

    #[test]
    fn supported_ordinary_media_file_names_include_images_and_videos() {
        assert!(is_supported_ordinary_media_file_name("photo.PNG"));
        assert!(is_supported_ordinary_media_file_name("raw.CR3"));
        assert!(is_supported_ordinary_media_file_name("clip.MP4"));
        assert!(is_supported_ordinary_media_file_name("clip.m4v"));
        assert!(is_supported_ordinary_media_file_name("clip.mov"));
    }

    #[test]
    fn supported_ordinary_media_file_names_exclude_archives_and_directories() {
        for name in [
            "book.cbz",
            "book.cbr",
            "archive.zip",
            "archive.tar",
            "archive.7z",
            "archive.rar",
            "directory",
            ".jpg",
            ".mp4",
        ] {
            assert!(!is_supported_ordinary_media_file_name(name), "{name}");
        }
    }
}
