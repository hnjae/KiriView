// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pub(crate) fn extension_for_file_name(name: &str) -> Option<String> {
    let dot_index = name.rfind('.')?;
    if dot_index == 0 || dot_index == name.len() - 1 {
        return None;
    }

    Some(name[dot_index + 1..].to_ascii_lowercase())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn extracts_lowercase_extension_from_file_names_and_entry_urls() {
        assert_eq!(extension_for_file_name("photo.JPG").as_deref(), Some("jpg"));
        assert_eq!(
            extension_for_file_name("zip:///path/book.cbz!/page.SVG").as_deref(),
            Some("svg")
        );
    }

    #[test]
    fn rejects_missing_hidden_only_and_trailing_dot_extensions() {
        assert_eq!(extension_for_file_name("photo"), None);
        assert_eq!(extension_for_file_name(".png"), None);
        assert_eq!(extension_for_file_name("photo."), None);
    }

    #[test]
    fn preserves_current_suffix_behavior_after_archive_root_dots() {
        assert_eq!(
            extension_for_file_name("zip:///path/book.cbz!/chapter/").as_deref(),
            Some("cbz!/chapter/")
        );
    }
}
