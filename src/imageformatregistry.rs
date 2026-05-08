// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

struct ImageFormat {
    extensions: &'static [&'static str],
    mime_types: &'static [&'static str],
}

struct ArchiveOpenProfile {
    extension: &'static str,
    mime_types: &'static [&'static str],
}

struct ArchiveFormat {
    scheme: &'static str,
    comic_book: ArchiveOpenProfile,
    direct_archive: ArchiveOpenProfile,
    backend: RustArchiveStorageBackend,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum ArchiveProfileSet {
    ComicBookOnly,
    DirectlyOpenable,
}

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
        extensions: &["svg"],
        mime_types: &["image/svg+xml"],
    },
];

const ARCHIVE_FORMATS: &[ArchiveFormat] = &[
    ArchiveFormat {
        scheme: "zip",
        comic_book: ArchiveOpenProfile {
            extension: "cbz",
            mime_types: &["application/vnd.comicbook+zip"],
        },
        direct_archive: ArchiveOpenProfile {
            extension: "zip",
            mime_types: &["application/zip"],
        },
        backend: RustArchiveStorageBackend::KZip,
    },
    ArchiveFormat {
        scheme: "tar",
        comic_book: ArchiveOpenProfile {
            extension: "cbt",
            mime_types: &["application/x-cbt"],
        },
        direct_archive: ArchiveOpenProfile {
            extension: "tar",
            mime_types: &["application/x-tar"],
        },
        backend: RustArchiveStorageBackend::KTar,
    },
    ArchiveFormat {
        scheme: "sevenz",
        comic_book: ArchiveOpenProfile {
            extension: "cb7",
            mime_types: &["application/x-cb7"],
        },
        direct_archive: ArchiveOpenProfile {
            extension: "7z",
            mime_types: &["application/x-7z-compressed"],
        },
        backend: RustArchiveStorageBackend::K7Zip,
    },
    ArchiveFormat {
        scheme: "rar",
        comic_book: ArchiveOpenProfile {
            extension: "cbr",
            mime_types: &["application/vnd.comicbook-rar", "application/x-cbr"],
        },
        direct_archive: ArchiveOpenProfile {
            extension: "rar",
            mime_types: &[
                "application/vnd.rar",
                "application/x-rar",
                "application/x-rar-compressed",
            ],
        },
        backend: RustArchiveStorageBackend::LibArchive,
    },
];

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    enum RustArchiveStorageBackend {
        None = 0,
        KZip = 1,
        KTar = 2,
        K7Zip = 3,
        LibArchive = 4,
    }

    enum RustArchiveOpenMatchKind {
        ComicBook = 0,
        GeneralArchive = 1,
    }

    struct RustArchiveOpenMatch {
        found: bool,
        scheme: String,
        kind: RustArchiveOpenMatchKind,
    }

    extern "Rust" {
        #[cxx_name = "rustSupportedImageExtensions"]
        fn rust_supported_image_extensions() -> Vec<String>;

        #[cxx_name = "rustSupportedImageMimeTypes"]
        fn rust_supported_image_mime_types() -> Vec<String>;

        #[cxx_name = "rustIsSupportedImageFileName"]
        fn rust_is_supported_image_file_name(name: &str) -> bool;

        #[cxx_name = "rustIsSupportedArchiveRootScheme"]
        fn rust_is_supported_archive_root_scheme(scheme: &str) -> bool;

        #[cxx_name = "rustArchiveStorageBackendForRootScheme"]
        fn rust_archive_storage_backend_for_root_scheme(scheme: &str) -> RustArchiveStorageBackend;

        #[cxx_name = "rustArchiveRootSchemeUsesKioFuse"]
        fn rust_archive_root_scheme_uses_kio_fuse(scheme: &str) -> bool;

        #[cxx_name = "rustSupportedComicBookArchiveExtensions"]
        fn rust_supported_comic_book_archive_extensions() -> Vec<String>;

        #[cxx_name = "rustSupportedComicBookArchiveMimeTypes"]
        fn rust_supported_comic_book_archive_mime_types() -> Vec<String>;

        #[cxx_name = "rustComicBookArchiveMatchForFileName"]
        fn rust_comic_book_archive_match_for_file_name(file_name: &str) -> RustArchiveOpenMatch;

        #[cxx_name = "rustDirectArchiveOpenMatchForFileName"]
        fn rust_direct_archive_open_match_for_file_name(file_name: &str) -> RustArchiveOpenMatch;

        #[cxx_name = "rustComicBookArchiveMatchForMimeTypeName"]
        fn rust_comic_book_archive_match_for_mime_type_name(
            mime_type_name: &str,
        ) -> RustArchiveOpenMatch;

        #[cxx_name = "rustDirectArchiveOpenMatchForMimeTypeName"]
        fn rust_direct_archive_open_match_for_mime_type_name(
            mime_type_name: &str,
        ) -> RustArchiveOpenMatch;

        #[cxx_name = "rustComicBookArchiveMarkerForRootScheme"]
        fn rust_comic_book_archive_marker_for_root_scheme(scheme: &str) -> String;

        #[cxx_name = "rustDirectArchiveOpenMarkersForRootScheme"]
        fn rust_direct_archive_open_markers_for_root_scheme(scheme: &str) -> Vec<String>;
    }
}

use ffi::{RustArchiveOpenMatch, RustArchiveOpenMatchKind, RustArchiveStorageBackend};

fn rust_supported_image_extensions() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_IMAGE_FORMATS
            .iter()
            .flat_map(|format| format.extensions.iter().copied()),
    )
}

fn rust_supported_image_mime_types() -> Vec<String> {
    unique_sorted_strings(
        SUPPORTED_IMAGE_FORMATS
            .iter()
            .flat_map(|format| format.mime_types.iter().copied()),
    )
}

fn rust_is_supported_image_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| image_extension_is_supported(extension.as_str()))
}

fn rust_is_supported_archive_root_scheme(scheme: &str) -> bool {
    archive_format_for_scheme(scheme).is_some()
}

fn rust_archive_storage_backend_for_root_scheme(scheme: &str) -> RustArchiveStorageBackend {
    archive_format_for_scheme(scheme)
        .map_or(RustArchiveStorageBackend::None, |format| format.backend)
}

fn rust_archive_root_scheme_uses_kio_fuse(scheme: &str) -> bool {
    storage_backend_uses_kio_fuse(rust_archive_storage_backend_for_root_scheme(scheme))
}

fn rust_supported_comic_book_archive_extensions() -> Vec<String> {
    ARCHIVE_FORMATS
        .iter()
        .map(|format| format.comic_book.extension.to_owned())
        .collect()
}

fn rust_supported_comic_book_archive_mime_types() -> Vec<String> {
    unique_sorted_strings(
        ARCHIVE_FORMATS
            .iter()
            .flat_map(|format| format.comic_book.mime_types.iter().copied()),
    )
}

fn rust_comic_book_archive_match_for_file_name(file_name: &str) -> RustArchiveOpenMatch {
    archive_match_for_file_name(file_name, ArchiveProfileSet::ComicBookOnly)
}

fn rust_direct_archive_open_match_for_file_name(file_name: &str) -> RustArchiveOpenMatch {
    archive_match_for_file_name(file_name, ArchiveProfileSet::DirectlyOpenable)
}

fn rust_comic_book_archive_match_for_mime_type_name(mime_type_name: &str) -> RustArchiveOpenMatch {
    archive_match_for_mime_type_name(mime_type_name, ArchiveProfileSet::ComicBookOnly)
}

fn rust_direct_archive_open_match_for_mime_type_name(mime_type_name: &str) -> RustArchiveOpenMatch {
    archive_match_for_mime_type_name(mime_type_name, ArchiveProfileSet::DirectlyOpenable)
}

fn rust_comic_book_archive_marker_for_root_scheme(scheme: &str) -> String {
    comic_book_archive_marker_for_root_scheme(scheme)
}

fn rust_direct_archive_open_markers_for_root_scheme(scheme: &str) -> Vec<String> {
    direct_archive_open_markers_for_root_scheme(scheme)
}

pub(crate) fn comic_book_archive_marker_for_root_scheme(scheme: &str) -> String {
    archive_format_for_scheme(scheme)
        .map_or_else(String::new, |format| marker_string(&format.comic_book))
}

pub(crate) fn direct_archive_open_markers_for_root_scheme(scheme: &str) -> Vec<String> {
    archive_format_for_scheme(scheme).map_or_else(Vec::new, |format| {
        vec![
            marker_string(&format.comic_book),
            marker_string(&format.direct_archive),
        ]
    })
}

fn image_extension_is_supported(extension: &str) -> bool {
    SUPPORTED_IMAGE_FORMATS
        .iter()
        .any(|format| format.extensions.contains(&extension))
}

fn archive_format_for_scheme(scheme: &str) -> Option<&'static ArchiveFormat> {
    ARCHIVE_FORMATS
        .iter()
        .find(|format| format.scheme == scheme)
}

fn accepted_profile_match(
    format: &ArchiveFormat,
    profile_set: ArchiveProfileSet,
    predicate: impl Fn(&ArchiveOpenProfile) -> bool,
) -> Option<RustArchiveOpenMatch> {
    if predicate(&format.comic_book) {
        return Some(archive_open_match(
            format.scheme,
            RustArchiveOpenMatchKind::ComicBook,
        ));
    }

    if profile_set == ArchiveProfileSet::DirectlyOpenable && predicate(&format.direct_archive) {
        return Some(archive_open_match(
            format.scheme,
            RustArchiveOpenMatchKind::GeneralArchive,
        ));
    }

    None
}

fn archive_match(
    profile_set: ArchiveProfileSet,
    predicate: impl Fn(&ArchiveOpenProfile) -> bool,
) -> RustArchiveOpenMatch {
    ARCHIVE_FORMATS
        .iter()
        .find_map(|format| accepted_profile_match(format, profile_set, &predicate))
        .unwrap_or_else(empty_archive_open_match)
}

fn archive_match_for_file_name(
    file_name: &str,
    profile_set: ArchiveProfileSet,
) -> RustArchiveOpenMatch {
    extension_for_file_name(file_name).map_or_else(empty_archive_open_match, |extension| {
        archive_match(profile_set, |profile| profile.extension == extension)
    })
}

fn archive_match_for_mime_type_name(
    mime_type_name: &str,
    profile_set: ArchiveProfileSet,
) -> RustArchiveOpenMatch {
    archive_match(profile_set, |profile| {
        profile.mime_types.contains(&mime_type_name)
    })
}

fn storage_backend_uses_kio_fuse(backend: RustArchiveStorageBackend) -> bool {
    matches!(
        backend,
        RustArchiveStorageBackend::KZip
            | RustArchiveStorageBackend::KTar
            | RustArchiveStorageBackend::K7Zip
    )
}

fn archive_open_match(scheme: &str, kind: RustArchiveOpenMatchKind) -> RustArchiveOpenMatch {
    RustArchiveOpenMatch {
        found: true,
        scheme: scheme.to_owned(),
        kind,
    }
}

fn empty_archive_open_match() -> RustArchiveOpenMatch {
    RustArchiveOpenMatch {
        found: false,
        scheme: String::new(),
        kind: RustArchiveOpenMatchKind::GeneralArchive,
    }
}

fn marker_string(profile: &ArchiveOpenProfile) -> String {
    format!(".{}/", profile.extension)
}

fn unique_sorted_strings(values: impl IntoIterator<Item = &'static str>) -> Vec<String> {
    let mut strings: Vec<String> = values.into_iter().map(str::to_owned).collect();
    strings.sort();
    strings.dedup();
    strings
}

fn extension_for_file_name(name: &str) -> Option<String> {
    let dot_index = name.rfind('.')?;
    if dot_index == 0 || dot_index == name.len() - 1 {
        return None;
    }

    Some(name[dot_index + 1..].to_ascii_lowercase())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn match_scheme(
        match_result: RustArchiveOpenMatch,
    ) -> Option<(String, RustArchiveOpenMatchKind)> {
        match_result
            .found
            .then_some((match_result.scheme, match_result.kind))
    }

    #[test]
    fn exposes_supported_archive_extensions_and_mime_types() {
        assert_eq!(
            rust_supported_comic_book_archive_extensions(),
            ["cbz", "cbt", "cb7", "cbr"]
        );
        assert_eq!(
            rust_supported_comic_book_archive_mime_types(),
            [
                "application/vnd.comicbook+zip",
                "application/vnd.comicbook-rar",
                "application/x-cb7",
                "application/x-cbr",
                "application/x-cbt",
            ]
        );
    }

    #[test]
    fn matches_archive_extensions_case_insensitively() {
        let comic = match_scheme(rust_comic_book_archive_match_for_file_name("book.CBZ"));
        assert!(matches!(
            comic,
            Some((scheme, RustArchiveOpenMatchKind::ComicBook)) if scheme == "zip"
        ));

        let direct = match_scheme(rust_direct_archive_open_match_for_file_name("book.7Z"));
        assert!(matches!(
            direct,
            Some((scheme, RustArchiveOpenMatchKind::GeneralArchive)) if scheme == "sevenz"
        ));

        assert!(match_scheme(rust_comic_book_archive_match_for_file_name("book.zip")).is_none());
        assert!(match_scheme(rust_direct_archive_open_match_for_file_name(".cbz")).is_none());
        assert!(match_scheme(rust_direct_archive_open_match_for_file_name("book.")).is_none());
    }

    #[test]
    fn matches_archive_mime_types_by_profile_set() {
        let comic = match_scheme(rust_direct_archive_open_match_for_mime_type_name(
            "application/x-cbr",
        ));
        assert!(matches!(
            comic,
            Some((scheme, RustArchiveOpenMatchKind::ComicBook)) if scheme == "rar"
        ));

        let general = match_scheme(rust_direct_archive_open_match_for_mime_type_name(
            "application/vnd.rar",
        ));
        assert!(matches!(
            general,
            Some((scheme, RustArchiveOpenMatchKind::GeneralArchive)) if scheme == "rar"
        ));

        assert!(
            match_scheme(rust_comic_book_archive_match_for_mime_type_name(
                "application/vnd.rar"
            ))
            .is_none()
        );
    }

    #[test]
    fn maps_archive_root_schemes_to_backend_and_markers() {
        assert!(rust_is_supported_archive_root_scheme("zip"));
        assert!(!rust_is_supported_archive_root_scheme("unknown"));
        assert!(matches!(
            rust_archive_storage_backend_for_root_scheme("tar"),
            RustArchiveStorageBackend::KTar
        ));
        assert!(rust_archive_root_scheme_uses_kio_fuse("sevenz"));
        assert!(!rust_archive_root_scheme_uses_kio_fuse("rar"));
        assert_eq!(
            rust_comic_book_archive_marker_for_root_scheme("zip"),
            ".cbz/"
        );
        assert_eq!(
            rust_direct_archive_open_markers_for_root_scheme("rar"),
            [".cbr/", ".rar/"]
        );
        assert!(rust_direct_archive_open_markers_for_root_scheme("unknown").is_empty());
    }
}
