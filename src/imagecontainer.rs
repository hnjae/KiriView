// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::{
    archivepath::{
        archive_relative_path_for_url_parts, clean_path, normalized_archive_root_path_for_path,
    },
    imageformatregistry::{
        comic_book_archive_marker_for_root_scheme, direct_archive_open_markers_for_root_scheme,
    },
};

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    struct RustArchiveRootPath {
        found: bool,
        path: String,
    }

    extern "Rust" {
        #[cxx_name = "rustArchiveRootPathForLocalArchive"]
        fn rust_archive_root_path_for_local_archive(
            is_local_file: bool,
            archive_scheme: &str,
            local_path: &str,
        ) -> RustArchiveRootPath;

        #[cxx_name = "rustContainingComicBookArchiveRootPath"]
        fn rust_containing_comic_book_archive_root_path(
            scheme: &str,
            path: &str,
        ) -> RustArchiveRootPath;

        #[cxx_name = "rustContainingDirectArchiveOpenRootPath"]
        fn rust_containing_direct_archive_open_root_path(
            scheme: &str,
            path: &str,
        ) -> RustArchiveRootPath;

        #[cxx_name = "rustArchiveDocumentContainsUrl"]
        fn rust_archive_document_contains_url(
            archive_document_empty: bool,
            archive_root_url_empty: bool,
            archive_root_scheme: &str,
            archive_root_path: &str,
            url_empty: bool,
            url_scheme: &str,
            url_path: &str,
        ) -> bool;

        #[cxx_name = "rustWindowTitleFileNameForDisplayedLocation"]
        fn rust_window_title_file_name_for_displayed_location(
            image_url_empty: bool,
            image_file_name: &str,
            displayed_location_inside_archive_document: bool,
            archive_file_name: &str,
        ) -> String;

        #[cxx_name = "rustZoomScopeUsesArchiveDocumentFileUrl"]
        fn rust_zoom_scope_uses_archive_document_file_url(
            displayed_location_inside_archive_document: bool,
        ) -> bool;

        #[cxx_name = "rustContainerNavigationUsesArchiveDocumentFileUrl"]
        fn rust_container_navigation_uses_archive_document_file_url(
            archive_document_is_comic_book: bool,
            displayed_location_inside_archive_document: bool,
        ) -> bool;

        #[cxx_name = "rustShouldResetRightToLeftReadingForLoad"]
        fn rust_should_reset_right_to_left_reading_for_load(
            right_to_left_reading_enabled: bool,
            container_navigation_url_empty: bool,
            displayed_archive_document_is_comic_book: bool,
            source_url_inside_displayed_archive_document: bool,
        ) -> bool;

        #[cxx_name = "rustComicArchiveReadingControlsAvailable"]
        fn rust_comic_archive_reading_controls_available(
            has_image: bool,
            displayed_url_empty: bool,
            displayed_archive_document_is_comic_book: bool,
        ) -> bool;
    }
}

use ffi::RustArchiveRootPath;

fn rust_archive_root_path_for_local_archive(
    is_local_file: bool,
    archive_scheme: &str,
    local_path: &str,
) -> RustArchiveRootPath {
    if !is_local_file || archive_scheme.is_empty() {
        return empty_archive_root_path();
    }

    let clean_local_path = clean_path(local_path);
    if clean_local_path.is_empty() {
        return empty_archive_root_path();
    }

    archive_root_path(normalized_archive_root_path_for_path(&clean_local_path))
}

fn rust_containing_comic_book_archive_root_path(scheme: &str, path: &str) -> RustArchiveRootPath {
    let marker = comic_book_archive_marker_for_root_scheme(scheme);
    if marker.is_empty() {
        return empty_archive_root_path();
    }

    containing_archive_root_path(path, &[marker])
}

fn rust_containing_direct_archive_open_root_path(scheme: &str, path: &str) -> RustArchiveRootPath {
    let markers = direct_archive_open_markers_for_root_scheme(scheme);
    containing_archive_root_path(path, &markers)
}

fn rust_archive_document_contains_url(
    archive_document_empty: bool,
    archive_root_url_empty: bool,
    archive_root_scheme: &str,
    archive_root_path: &str,
    url_empty: bool,
    url_scheme: &str,
    url_path: &str,
) -> bool {
    !archive_document_empty
        && !archive_relative_path_for_url_parts(
            archive_root_url_empty,
            archive_root_scheme,
            archive_root_path,
            url_empty,
            url_scheme,
            url_path,
        )
        .is_empty()
}

fn rust_window_title_file_name_for_displayed_location(
    image_url_empty: bool,
    image_file_name: &str,
    displayed_location_inside_archive_document: bool,
    archive_file_name: &str,
) -> String {
    if image_url_empty {
        return String::new();
    }

    if displayed_location_inside_archive_document && !archive_file_name.is_empty() {
        return archive_file_name.to_owned();
    }

    image_file_name.to_owned()
}

fn rust_zoom_scope_uses_archive_document_file_url(
    displayed_location_inside_archive_document: bool,
) -> bool {
    displayed_location_inside_archive_document
}

fn rust_container_navigation_uses_archive_document_file_url(
    archive_document_is_comic_book: bool,
    displayed_location_inside_archive_document: bool,
) -> bool {
    archive_document_is_comic_book && displayed_location_inside_archive_document
}

fn rust_should_reset_right_to_left_reading_for_load(
    right_to_left_reading_enabled: bool,
    container_navigation_url_empty: bool,
    displayed_archive_document_is_comic_book: bool,
    source_url_inside_displayed_archive_document: bool,
) -> bool {
    right_to_left_reading_enabled
        && container_navigation_url_empty
        && (!displayed_archive_document_is_comic_book
            || !source_url_inside_displayed_archive_document)
}

fn rust_comic_archive_reading_controls_available(
    has_image: bool,
    displayed_url_empty: bool,
    displayed_archive_document_is_comic_book: bool,
) -> bool {
    has_image && !displayed_url_empty && displayed_archive_document_is_comic_book
}

fn containing_archive_root_path(path: &str, markers: &[String]) -> RustArchiveRootPath {
    if markers.is_empty() {
        return empty_archive_root_path();
    }

    let path = clean_path(path);
    let mut selected_marker: Option<(usize, usize)> = None;
    for marker in markers {
        let Some(candidate_index) = find_ascii_case_insensitive(&path, marker) else {
            continue;
        };
        if selected_marker.is_none_or(|(marker_index, _)| candidate_index < marker_index) {
            selected_marker = Some((candidate_index, marker.len()));
        }
    }

    let Some((marker_index, marker_size)) = selected_marker else {
        return empty_archive_root_path();
    };
    let root_end = marker_index + marker_size - 1;
    archive_root_path(format!("{}/", &path[..root_end]))
}

fn find_ascii_case_insensitive(haystack: &str, needle: &str) -> Option<usize> {
    let haystack = haystack.as_bytes();
    let needle = needle.as_bytes();
    if needle.is_empty() || needle.len() > haystack.len() {
        return None;
    }

    haystack
        .windows(needle.len())
        .position(|window| window.eq_ignore_ascii_case(needle))
}

fn archive_root_path(path: String) -> RustArchiveRootPath {
    RustArchiveRootPath { found: true, path }
}

fn empty_archive_root_path() -> RustArchiveRootPath {
    RustArchiveRootPath {
        found: false,
        path: String::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn builds_archive_root_paths_for_local_archives() {
        let root = rust_archive_root_path_for_local_archive(true, "zip", "/books/./book.cbz");
        assert!(root.found);
        assert_eq!(root.path, "/books/book.cbz/");

        assert!(!rust_archive_root_path_for_local_archive(false, "zip", "/books/book.cbz").found);
        assert!(!rust_archive_root_path_for_local_archive(true, "", "/books/book.cbz").found);
    }

    #[test]
    fn resolves_containing_archive_root_paths() {
        let comic = rust_containing_comic_book_archive_root_path(
            "zip",
            "/books/Book.CBZ/chapter/page001.png",
        );
        assert!(comic.found);
        assert_eq!(comic.path, "/books/Book.CBZ/");

        let direct = rust_containing_direct_archive_open_root_path(
            "rar",
            "/books/book.rar/chapter/page001.png",
        );
        assert!(direct.found);
        assert_eq!(direct.path, "/books/book.rar/");

        assert!(
            !rust_containing_comic_book_archive_root_path(
                "zip",
                "/books/book.zip/chapter/page001.png"
            )
            .found
        );
    }

    #[test]
    fn archive_scope_decisions_require_archive_containment() {
        assert!(rust_archive_document_contains_url(
            false,
            false,
            "zip",
            "/books/book.cbz/",
            false,
            "zip",
            "/books/book.cbz/chapter/page001.png"
        ));
        assert!(!rust_archive_document_contains_url(
            false,
            false,
            "zip",
            "/books/book.cbz/",
            false,
            "tar",
            "/books/book.cbz/chapter/page001.png"
        ));
        assert!(rust_zoom_scope_uses_archive_document_file_url(true));
        assert!(rust_container_navigation_uses_archive_document_file_url(
            true, true
        ));
        assert!(!rust_container_navigation_uses_archive_document_file_url(
            false, true
        ));
    }

    #[test]
    fn right_to_left_reading_reset_requires_leaving_a_comic_archive_directly() {
        assert!(!rust_should_reset_right_to_left_reading_for_load(
            false, true, true, false,
        ));
        assert!(!rust_should_reset_right_to_left_reading_for_load(
            true, false, true, false,
        ));
        assert!(!rust_should_reset_right_to_left_reading_for_load(
            true, true, true, true,
        ));
        assert!(rust_should_reset_right_to_left_reading_for_load(
            true, true, true, false,
        ));
        assert!(rust_should_reset_right_to_left_reading_for_load(
            true, true, false, false,
        ));
    }

    #[test]
    fn comic_archive_reading_controls_require_displayed_comic_archive_image() {
        assert!(rust_comic_archive_reading_controls_available(
            true, false, true,
        ));
        assert!(!rust_comic_archive_reading_controls_available(
            false, false, true,
        ));
        assert!(!rust_comic_archive_reading_controls_available(
            true, true, true,
        ));
        assert!(!rust_comic_archive_reading_controls_available(
            true, false, false,
        ));
    }

    #[test]
    fn chooses_archive_name_for_archive_window_titles() {
        assert_eq!(
            rust_window_title_file_name_for_displayed_location(
                false,
                "page001.png",
                true,
                "book.cbz",
            ),
            "book.cbz"
        );
        assert_eq!(
            rust_window_title_file_name_for_displayed_location(false, "page001.png", false, "",),
            "page001.png"
        );
    }
}
