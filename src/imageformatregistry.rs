// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const SUPPORTED_IMAGE_EXTENSIONS: &[&str] = &[
    "avci", "avif", "avifs", "bmp", "gif", "hej2", "heic", "heics", "heif", "heifs", "hif", "jpeg",
    "jpg", "jp2", "jxl", "png", "svg", "webp",
];

struct ArchiveFormat {
    scheme: &'static str,
    comic_book_extension: &'static str,
    direct_archive_extension: &'static str,
    comic_book_mime_types: &'static [&'static str],
    direct_archive_mime_types: &'static [&'static str],
}

const ARCHIVE_FORMATS: &[ArchiveFormat] = &[
    ArchiveFormat {
        scheme: "zip",
        comic_book_extension: "cbz",
        direct_archive_extension: "zip",
        comic_book_mime_types: &["application/vnd.comicbook+zip"],
        direct_archive_mime_types: &["application/zip"],
    },
    ArchiveFormat {
        scheme: "tar",
        comic_book_extension: "cbt",
        direct_archive_extension: "tar",
        comic_book_mime_types: &["application/x-cbt"],
        direct_archive_mime_types: &["application/x-tar"],
    },
    ArchiveFormat {
        scheme: "sevenz",
        comic_book_extension: "cb7",
        direct_archive_extension: "7z",
        comic_book_mime_types: &["application/x-cb7"],
        direct_archive_mime_types: &["application/x-7z-compressed"],
    },
    ArchiveFormat {
        scheme: "rar",
        comic_book_extension: "cbr",
        direct_archive_extension: "rar",
        comic_book_mime_types: &["application/vnd.comicbook-rar", "application/x-cbr"],
        direct_archive_mime_types: &[
            "application/vnd.rar",
            "application/x-rar",
            "application/x-rar-compressed",
        ],
    },
];

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustSupportedImageExtensions"]
        fn rust_supported_image_extensions() -> Vec<String>;

        #[cxx_name = "rustSupportedOpenExtensions"]
        fn rust_supported_open_extensions() -> Vec<String>;

        #[cxx_name = "rustIsSupportedImageFileName"]
        fn rust_is_supported_image_file_name(name: &str) -> bool;

        #[cxx_name = "rustIsComicBookArchiveFileName"]
        fn rust_is_comic_book_archive_file_name(name: &str) -> bool;

        #[cxx_name = "rustComicBookArchiveKioSchemeForFileName"]
        fn rust_comic_book_archive_kio_scheme_for_file_name(name: &str) -> String;

        #[cxx_name = "rustDirectArchiveOpenKioSchemeForFileName"]
        fn rust_direct_archive_open_kio_scheme_for_file_name(name: &str) -> String;

        #[cxx_name = "rustComicBookArchiveKioSchemeForMimeTypeName"]
        fn rust_comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name: &str) -> String;

        #[cxx_name = "rustDirectArchiveOpenKioSchemeForMimeTypeName"]
        fn rust_direct_archive_open_kio_scheme_for_mime_type_name(mime_type_name: &str) -> String;
    }
}

fn rust_supported_image_extensions() -> Vec<String> {
    strings(SUPPORTED_IMAGE_EXTENSIONS)
}

fn rust_supported_open_extensions() -> Vec<String> {
    let mut extensions = strings(SUPPORTED_IMAGE_EXTENSIONS);
    extensions.extend(
        ARCHIVE_FORMATS
            .iter()
            .map(|format| format.comic_book_extension.to_owned()),
    );
    extensions.sort();
    extensions
}

fn rust_is_supported_image_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| SUPPORTED_IMAGE_EXTENSIONS.contains(&extension.as_str()))
}

fn rust_is_comic_book_archive_file_name(name: &str) -> bool {
    !rust_comic_book_archive_kio_scheme_for_file_name(name).is_empty()
}

fn rust_comic_book_archive_kio_scheme_for_file_name(name: &str) -> String {
    extension_for_file_name(name)
        .as_deref()
        .and_then(comic_book_archive_kio_scheme_for_extension)
        .unwrap_or_default()
        .to_owned()
}

fn rust_direct_archive_open_kio_scheme_for_file_name(name: &str) -> String {
    extension_for_file_name(name)
        .as_deref()
        .and_then(direct_archive_open_kio_scheme_for_extension)
        .unwrap_or_default()
        .to_owned()
}

fn rust_comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name: &str) -> String {
    comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name)
        .unwrap_or_default()
        .to_owned()
}

fn rust_direct_archive_open_kio_scheme_for_mime_type_name(mime_type_name: &str) -> String {
    direct_archive_open_kio_scheme_for_mime_type_name(mime_type_name)
        .unwrap_or_default()
        .to_owned()
}

fn strings(values: &[&str]) -> Vec<String> {
    values.iter().map(|value| (*value).to_owned()).collect()
}

fn extension_for_file_name(name: &str) -> Option<String> {
    let dot_index = name.rfind('.')?;
    if dot_index == 0 || dot_index == name.len() - 1 {
        return None;
    }

    Some(name[dot_index + 1..].to_ascii_lowercase())
}

fn comic_book_archive_kio_scheme_for_extension(extension: &str) -> Option<&'static str> {
    archive_format_for_comic_book_extension(extension).map(|format| format.scheme)
}

fn direct_archive_open_kio_scheme_for_extension(extension: &str) -> Option<&'static str> {
    comic_book_archive_kio_scheme_for_extension(extension).or_else(|| {
        ARCHIVE_FORMATS
            .iter()
            .find(|format| format.direct_archive_extension == extension)
            .map(|format| format.scheme)
    })
}

fn comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name: &str) -> Option<&'static str> {
    ARCHIVE_FORMATS
        .iter()
        .find(|format| format.comic_book_mime_types.contains(&mime_type_name))
        .map(|format| format.scheme)
}

fn direct_archive_open_kio_scheme_for_mime_type_name(mime_type_name: &str) -> Option<&'static str> {
    comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name).or_else(|| {
        ARCHIVE_FORMATS
            .iter()
            .find(|format| format.direct_archive_mime_types.contains(&mime_type_name))
            .map(|format| format.scheme)
    })
}

fn archive_format_for_comic_book_extension(extension: &str) -> Option<&'static ArchiveFormat> {
    ARCHIVE_FORMATS
        .iter()
        .find(|format| format.comic_book_extension == extension)
}
