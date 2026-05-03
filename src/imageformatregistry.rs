// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const SUPPORTED_IMAGE_EXTENSIONS: &[&str] = &[
    "avci", "avif", "avifs", "bmp", "gif", "hej2", "heic", "heics", "heif", "heifs", "hif", "jpeg",
    "jpg", "jp2", "jxl", "png", "svg", "webp",
];

const COMIC_BOOK_ARCHIVE_EXTENSIONS: &[&str] = &["cbz", "cbt", "cb7", "cbr"];

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
    extensions.extend(strings(COMIC_BOOK_ARCHIVE_EXTENSIONS));
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
    match extension {
        "cbz" => Some("zip"),
        "cbt" => Some("tar"),
        "cb7" => Some("sevenz"),
        "cbr" => Some("rar"),
        _ => None,
    }
}

fn direct_archive_open_kio_scheme_for_extension(extension: &str) -> Option<&'static str> {
    comic_book_archive_kio_scheme_for_extension(extension).or(match extension {
        "zip" => Some("zip"),
        "tar" => Some("tar"),
        "7z" => Some("sevenz"),
        "rar" => Some("rar"),
        _ => None,
    })
}

fn comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name: &str) -> Option<&'static str> {
    match mime_type_name {
        "application/vnd.comicbook+zip" => Some("zip"),
        "application/x-cbt" => Some("tar"),
        "application/x-cb7" => Some("sevenz"),
        "application/vnd.comicbook-rar" | "application/x-cbr" => Some("rar"),
        _ => None,
    }
}

fn direct_archive_open_kio_scheme_for_mime_type_name(mime_type_name: &str) -> Option<&'static str> {
    comic_book_archive_kio_scheme_for_mime_type_name(mime_type_name).or(match mime_type_name {
        "application/zip" => Some("zip"),
        "application/x-tar" => Some("tar"),
        "application/x-7z-compressed" => Some("sevenz"),
        "application/vnd.rar" | "application/x-rar" | "application/x-rar-compressed" => Some("rar"),
        _ => None,
    })
}
