// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const SUPPORTED_IMAGE_EXTENSIONS: &[&str] = &[
    "avci", "avif", "avifs", "bmp", "gif", "hej2", "heic", "heics", "heif", "heifs", "hif", "jpeg",
    "jpg", "jp2", "jxl", "png", "svg", "webp",
];

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustSupportedImageExtensions"]
        fn rust_supported_image_extensions() -> Vec<String>;

        #[cxx_name = "rustIsSupportedImageFileName"]
        fn rust_is_supported_image_file_name(name: &str) -> bool;
    }
}

fn rust_supported_image_extensions() -> Vec<String> {
    strings(SUPPORTED_IMAGE_EXTENSIONS)
}

fn rust_is_supported_image_file_name(name: &str) -> bool {
    extension_for_file_name(name)
        .is_some_and(|extension| SUPPORTED_IMAGE_EXTENSIONS.contains(&extension.as_str()))
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
