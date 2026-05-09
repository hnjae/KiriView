// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::{archivepath::clean_path, imageformatregistry::archive_root_scheme_uses_kio_fuse};

const KIO_FUSE_MARKER: &str = "/kio-fuse-";

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    struct RustKioFuseArchivePath {
        found: bool,
        scheme: String,
        path: String,
    }

    extern "Rust" {
        #[cxx_name = "rustKioFuseArchivePath"]
        fn rust_kio_fuse_archive_path(
            local_path: &str,
            runtime_dir: &str,
        ) -> RustKioFuseArchivePath;
    }
}

use ffi::RustKioFuseArchivePath;

fn rust_kio_fuse_archive_path(local_path: &str, runtime_dir: &str) -> RustKioFuseArchivePath {
    kio_fuse_archive_path(local_path, runtime_dir)
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

    fn parsed(local_path: &str, runtime_dir: &str) -> Option<(String, String)> {
        let result = kio_fuse_archive_path(local_path, runtime_dir);
        result.found.then_some((result.scheme, result.path))
    }

    #[test]
    fn restores_archive_scheme_paths_from_runtime_limited_kio_fuse_mounts() {
        assert_eq!(
            parsed(
                "/run/user/1000/kio-fuse-test/zip/books/book.cbz/page.png",
                "/run/user/1000"
            ),
            Some(("zip".to_owned(), "/books/book.cbz/page.png".to_owned()))
        );
        assert_eq!(
            parsed(
                "/run/user/1000/./kio-fuse-test/tar/books/book.cbt/page.png",
                "/run/user/1000/"
            ),
            Some(("tar".to_owned(), "/books/book.cbt/page.png".to_owned()))
        );
    }

    #[test]
    fn finds_kio_fuse_mounts_without_runtime_dir() {
        assert_eq!(
            parsed("/tmp/kio-fuse-test/sevenz/books/book.cb7/page.png", ""),
            Some(("sevenz".to_owned(), "/books/book.cb7/page.png".to_owned()))
        );
    }

    #[test]
    fn rejects_paths_outside_runtime_dir_when_runtime_dir_is_known() {
        assert_eq!(
            parsed(
                "/tmp/kio-fuse-test/zip/books/book.cbz/page.png",
                "/run/user/1000"
            ),
            None
        );
    }

    #[test]
    fn rejects_unknown_or_incomplete_kio_fuse_paths() {
        assert_eq!(
            parsed(
                "/run/user/1000/kio-fuse-test/rar/books/book.rar/page.png",
                "/run/user/1000"
            ),
            None
        );
        assert_eq!(
            parsed("/run/user/1000/kio-fuse-test/zip", "/run/user/1000"),
            None
        );
        assert_eq!(
            parsed("/run/user/1000/kio-fuse-test/zip/", "/run/user/1000"),
            None
        );
        assert_eq!(
            parsed(
                "/run/user/1000/kio-fuse-test//books/book.cbz",
                "/run/user/1000"
            ),
            None
        );
    }
}
