// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustNormalizedArchiveRootPath"]
        fn rust_normalized_archive_root_path(path: &str) -> String;

        #[cxx_name = "rustNormalizedArchiveEntryPath"]
        fn rust_normalized_archive_entry_path(entry_path: &str) -> String;

        #[cxx_name = "rustArchiveRelativePathForUrl"]
        fn rust_archive_relative_path_for_url(
            archive_root_url_empty: bool,
            archive_root_scheme: &str,
            archive_root_path: &str,
            url_empty: bool,
            url_scheme: &str,
            url_path: &str,
        ) -> String;
    }
}

fn rust_normalized_archive_root_path(path: &str) -> String {
    normalized_archive_root_path_for_path(path)
}

fn rust_normalized_archive_entry_path(entry_path: &str) -> String {
    normalized_archive_entry_path_for_path(entry_path)
}

fn rust_archive_relative_path_for_url(
    archive_root_url_empty: bool,
    archive_root_scheme: &str,
    archive_root_path: &str,
    url_empty: bool,
    url_scheme: &str,
    url_path: &str,
) -> String {
    archive_relative_path_for_url_parts(
        archive_root_url_empty,
        archive_root_scheme,
        archive_root_path,
        url_empty,
        url_scheme,
        url_path,
    )
}

pub(crate) fn normalized_archive_root_path_for_path(path: &str) -> String {
    archive_root_path_with_trailing_slash(clean_path(path))
}

pub(crate) fn normalized_archive_entry_path_for_path(entry_path: &str) -> String {
    let mut clean_path = clean_path(entry_path);
    while clean_path.starts_with("./") {
        clean_path.replace_range(0..2, "");
    }

    if clean_path == "."
        || clean_path == ".."
        || clean_path.starts_with("../")
        || clean_path.starts_with('/')
    {
        return String::new();
    }

    clean_path
}

pub(crate) fn archive_relative_path_for_url_parts(
    archive_root_url_empty: bool,
    archive_root_scheme: &str,
    archive_root_path: &str,
    url_empty: bool,
    url_scheme: &str,
    url_path: &str,
) -> String {
    if url_empty || archive_root_url_empty || url_scheme != archive_root_scheme {
        return String::new();
    }

    let root_path = normalized_archive_root_path_for_path(archive_root_path);
    let path = clean_path(url_path);
    if path.len() <= root_path.len() || !path.starts_with(&root_path) {
        return String::new();
    }

    path[root_path.len()..].to_owned()
}

pub(crate) fn clean_path(path: &str) -> String {
    let is_absolute = path.starts_with('/');
    let mut parts = Vec::new();

    for part in path.split('/') {
        match part {
            "" | "." => {}
            ".." => {
                if parts.last().is_some_and(|last| *last != "..") {
                    parts.pop();
                } else if !is_absolute {
                    parts.push(part);
                }
            }
            _ => parts.push(part),
        }
    }

    if parts.is_empty() {
        return if is_absolute { "/" } else { "." }.to_owned();
    }

    let clean_path = parts.join("/");
    if is_absolute {
        format!("/{clean_path}")
    } else {
        clean_path
    }
}

fn archive_root_path_with_trailing_slash(mut path: String) -> String {
    if !path.ends_with('/') {
        path.push('/');
    }

    path
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cleans_paths_like_qdir_clean_path_for_archive_inputs() {
        assert_eq!(clean_path("./chapter/./page001.png"), "chapter/page001.png");
        assert_eq!(
            clean_path("/books/book.cbz/../page001.png"),
            "/books/page001.png"
        );
        assert_eq!(clean_path("chapter//page001.png"), "chapter/page001.png");
        assert_eq!(clean_path("."), ".");
        assert_eq!(clean_path(".."), "..");
        assert_eq!(clean_path("/.."), "/");
    }

    #[test]
    fn normalizes_archive_root_paths_with_trailing_slash() {
        assert_eq!(
            normalized_archive_root_path_for_path("/books/./book.cbz"),
            "/books/book.cbz/"
        );
        assert_eq!(
            normalized_archive_root_path_for_path("/books/book.cbz/"),
            "/books/book.cbz/"
        );
    }

    #[test]
    fn rejects_unsafe_entry_paths() {
        assert_eq!(
            normalized_archive_entry_path_for_path("./chapter/./page001.png"),
            "chapter/page001.png"
        );
        assert!(normalized_archive_entry_path_for_path("../page001.png").is_empty());
        assert!(normalized_archive_entry_path_for_path("chapter/../../page001.png").is_empty());
        assert!(normalized_archive_entry_path_for_path("/tmp/page001.png").is_empty());
        assert!(normalized_archive_entry_path_for_path(".").is_empty());
        assert!(normalized_archive_entry_path_for_path("..").is_empty());
    }

    #[test]
    fn calculates_archive_relative_paths_only_inside_root() {
        assert_eq!(
            archive_relative_path_for_url_parts(
                false,
                "zip",
                "/books/book.cbz/",
                false,
                "zip",
                "/books/book.cbz/chapter/page001.png"
            ),
            "chapter/page001.png"
        );
        assert!(
            archive_relative_path_for_url_parts(
                false,
                "zip",
                "/books/book.cbz/",
                false,
                "zip",
                "/books/book.cbz/"
            )
            .is_empty()
        );
        assert!(
            archive_relative_path_for_url_parts(
                false,
                "zip",
                "/books/book.cbz/",
                false,
                "zip",
                "/books/book.cbz/../page001.png"
            )
            .is_empty()
        );
        assert!(
            archive_relative_path_for_url_parts(
                false,
                "zip",
                "/books/book.cbz/",
                false,
                "tar",
                "/books/book.cbz/chapter/page001.png"
            )
            .is_empty()
        );
    }
}
