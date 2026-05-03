// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{QString, QUrl};
use std::{
    ffi::OsStr,
    path::{Path, PathBuf},
};

mod apngdecoder;
mod avifcompat;
mod imagenavigationmodel;
mod imageviewportgeometry;
mod imagezoomstate;

pub fn initialize_rust_modules() {
    let _ = core::mem::size_of::<cxx_qt_lib_extras::QApplication>();
}

pub fn initial_source_url_from_args(
    args: impl IntoIterator<Item = impl AsRef<OsStr>>,
    working_directory: Option<&Path>,
) -> Option<QUrl> {
    args.into_iter()
        .skip(1)
        .find(|argument| argument.as_ref() != OsStr::new("--"))
        .and_then(|argument| source_url_from_argument(argument.as_ref(), working_directory))
}

fn source_url_from_argument(argument: &OsStr, working_directory: Option<&Path>) -> Option<QUrl> {
    let argument = argument.to_string_lossy();
    if argument.is_empty() {
        return None;
    }

    let url = QUrl::from(&QString::from(argument.as_ref()));
    if !url.is_relative() {
        return valid_initial_source_url(url);
    }

    let path = PathBuf::from(argument.as_ref());
    let absolute_path = if path.is_absolute() {
        path
    } else if let Some(working_directory) = working_directory {
        working_directory.join(path)
    } else {
        path
    };
    let local_url = QUrl::from_local_file(&QString::from(absolute_path.to_string_lossy().as_ref()));
    valid_initial_source_url(local_url)
}

fn valid_initial_source_url(url: QUrl) -> Option<QUrl> {
    if url.is_empty() || !url.is_valid() {
        None
    } else {
        Some(url)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::ffi::OsString;

    fn startup_args(arguments: &[&str]) -> Vec<OsString> {
        std::iter::once(OsString::from("kiriview"))
            .chain(arguments.iter().map(OsString::from))
            .collect()
    }

    fn local_file_path(url: &QUrl) -> Option<String> {
        url.to_local_file().map(String::from)
    }

    #[test]
    fn relative_startup_path_becomes_local_file_url() {
        let url = initial_source_url_from_args(
            startup_args(&["archive-zip.cbt"]),
            Some(Path::new("/home/user/books")),
        )
        .expect("relative startup path should produce a URL");

        assert_eq!(
            local_file_path(&url).as_deref(),
            Some("/home/user/books/archive-zip.cbt")
        );
    }

    #[test]
    fn startup_path_after_separator_becomes_local_file_url() {
        let url = initial_source_url_from_args(
            startup_args(&["--", "archive-zip.cbt"]),
            Some(Path::new("/home/user/books")),
        )
        .expect("startup path after -- should produce a URL");

        assert_eq!(
            local_file_path(&url).as_deref(),
            Some("/home/user/books/archive-zip.cbt")
        );
    }

    #[test]
    fn startup_url_argument_keeps_its_scheme() {
        let url = initial_source_url_from_args(
            startup_args(&["smb://server/share/page.png"]),
            Some(Path::new("/home/user/books")),
        )
        .expect("startup URL should produce a URL");

        assert_eq!(String::from(url.scheme_or_default()), "smb");
        assert_eq!(String::from(url.host_or_default()), "server");
        assert_eq!(String::from(url.path()), "/share/page.png");
    }

    #[test]
    fn startup_separator_without_path_produces_no_url() {
        assert!(
            initial_source_url_from_args(
                startup_args(&["--"]),
                Some(Path::new("/home/user/books"))
            )
            .is_none()
        );
    }
}
