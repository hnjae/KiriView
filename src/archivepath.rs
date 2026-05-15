// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::archiveformat::archive_root_scheme_uses_kio_fuse;

const KIO_FUSE_MARKER: &str = "/kio-fuse-";

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    struct RustKioFuseArchivePath {
        found: bool,
        scheme: String,
        path: String,
    }

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

        #[cxx_name = "rustKioFuseArchivePath"]
        fn rust_kio_fuse_archive_path(
            local_path: &str,
            runtime_dir: &str,
        ) -> RustKioFuseArchivePath;
    }
}

use ffi::RustKioFuseArchivePath;

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

fn rust_kio_fuse_archive_path(local_path: &str, runtime_dir: &str) -> RustKioFuseArchivePath {
    kio_fuse_archive_path(local_path, runtime_dir)
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

fn kio_fuse_archive_path(local_path: &str, runtime_dir: &str) -> RustKioFuseArchivePath {
    let clean_local_path = clean_path(local_path);
    let Some(marker_index) = kio_fuse_marker_index(&clean_local_path, runtime_dir) else {
        return empty_kio_fuse_archive_path();
    };

    let mount_name_start = marker_index + KIO_FUSE_MARKER.len();
    let Some(mount_name_end) = clean_local_path[mount_name_start..].find('/') else {
        return empty_kio_fuse_archive_path();
    };
    let mount_end = mount_name_start + mount_name_end;
    if mount_end == clean_local_path.len() - 1 {
        return empty_kio_fuse_archive_path();
    }

    let relative_path = &clean_local_path[mount_end + 1..];
    let Some(scheme_end) = relative_path.find('/') else {
        return empty_kio_fuse_archive_path();
    };
    if scheme_end == 0 || scheme_end == relative_path.len() - 1 {
        return empty_kio_fuse_archive_path();
    }

    let scheme = &relative_path[..scheme_end];
    if !archive_root_scheme_uses_kio_fuse(scheme) {
        return empty_kio_fuse_archive_path();
    }

    kio_fuse_archive_path_result(scheme.to_owned(), relative_path[scheme_end..].to_owned())
}

fn kio_fuse_marker_index(clean_local_path: &str, runtime_dir: &str) -> Option<usize> {
    if runtime_dir.is_empty() {
        return clean_local_path.find(KIO_FUSE_MARKER);
    }

    let prefix = format!("{}{}", clean_path(runtime_dir), KIO_FUSE_MARKER);
    clean_local_path
        .starts_with(&prefix)
        .then_some(prefix.len() - KIO_FUSE_MARKER.len())
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

fn kio_fuse_archive_path_result(scheme: String, path: String) -> RustKioFuseArchivePath {
    RustKioFuseArchivePath {
        found: true,
        scheme,
        path,
    }
}

fn empty_kio_fuse_archive_path() -> RustKioFuseArchivePath {
    RustKioFuseArchivePath {
        found: false,
        scheme: String::new(),
        path: String::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn parsed_kio_fuse_archive_path(
        local_path: &str,
        runtime_dir: &str,
    ) -> Option<(String, String)> {
        let result = kio_fuse_archive_path(local_path, runtime_dir);
        result.found.then_some((result.scheme, result.path))
    }

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

    #[test]
    fn restores_archive_scheme_paths_from_runtime_limited_kio_fuse_mounts() {
        assert_eq!(
            parsed_kio_fuse_archive_path(
                "/run/user/1000/kio-fuse-test/zip/books/book.cbz/page.png",
                "/run/user/1000"
            ),
            Some(("zip".to_owned(), "/books/book.cbz/page.png".to_owned()))
        );
        assert_eq!(
            parsed_kio_fuse_archive_path(
                "/run/user/1000/./kio-fuse-test/tar/books/book.cbt/page.png",
                "/run/user/1000/"
            ),
            Some(("tar".to_owned(), "/books/book.cbt/page.png".to_owned()))
        );
    }

    #[test]
    fn finds_kio_fuse_mounts_without_runtime_dir() {
        assert_eq!(
            parsed_kio_fuse_archive_path("/tmp/kio-fuse-test/sevenz/books/book.cb7/page.png", ""),
            Some(("sevenz".to_owned(), "/books/book.cb7/page.png".to_owned()))
        );
    }

    #[test]
    fn rejects_paths_outside_runtime_dir_when_runtime_dir_is_known() {
        assert_eq!(
            parsed_kio_fuse_archive_path(
                "/tmp/kio-fuse-test/zip/books/book.cbz/page.png",
                "/run/user/1000"
            ),
            None
        );
    }

    #[test]
    fn rejects_unknown_or_incomplete_kio_fuse_paths() {
        assert_eq!(
            parsed_kio_fuse_archive_path(
                "/run/user/1000/kio-fuse-test/rar/books/book.rar/page.png",
                "/run/user/1000"
            ),
            None
        );
        assert_eq!(
            parsed_kio_fuse_archive_path("/run/user/1000/kio-fuse-test/zip", "/run/user/1000"),
            None
        );
        assert_eq!(
            parsed_kio_fuse_archive_path("/run/user/1000/kio-fuse-test/zip/", "/run/user/1000"),
            None
        );
        assert_eq!(
            parsed_kio_fuse_archive_path(
                "/run/user/1000/kio-fuse-test//books/book.cbz",
                "/run/user/1000"
            ),
            None
        );
    }
}
