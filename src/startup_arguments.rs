// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{QString, QUrl};
use std::{
    ffi::OsStr,
    fmt,
    path::{Path, PathBuf},
};

pub const STARTUP_ARGUMENT_ERROR_EXIT_CODE: i32 = 2;

const MISSING_LOCAL_FILE_REASON: &str = "No such file or directory";

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct StartupArgumentError {
    path: PathBuf,
    reason: String,
}

impl StartupArgumentError {
    pub fn path(&self) -> &Path {
        &self.path
    }

    pub fn reason(&self) -> &str {
        &self.reason
    }
}

impl fmt::Display for StartupArgumentError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            formatter,
            "cannot open '{}': {}",
            self.path.display(),
            self.reason
        )
    }
}

pub fn initial_source_url_from_args(
    args: impl IntoIterator<Item = impl AsRef<OsStr>>,
    working_directory: Option<&Path>,
) -> Result<Option<QUrl>, StartupArgumentError> {
    let Some(argument) = args
        .into_iter()
        .skip(1)
        .find(|argument| argument.as_ref() != OsStr::new("--"))
    else {
        return Ok(None);
    };

    source_url_from_argument(argument.as_ref(), working_directory)
}

fn source_url_from_argument(
    argument: &OsStr,
    working_directory: Option<&Path>,
) -> Result<Option<QUrl>, StartupArgumentError> {
    let argument = argument.to_string_lossy();
    if argument.is_empty() {
        return Ok(None);
    }

    let url = QUrl::from(&QString::from(argument.as_ref()));
    if !url.is_relative() {
        validate_local_url(&url)?;
        return Ok(valid_initial_source_url(url));
    }

    let path = PathBuf::from(argument.as_ref());
    let absolute_path = if path.is_absolute() {
        path
    } else if let Some(working_directory) = working_directory {
        working_directory.join(path)
    } else {
        path
    };
    validate_local_file_path(&absolute_path)?;

    let local_url = QUrl::from_local_file(&QString::from(absolute_path.to_string_lossy().as_ref()));
    Ok(valid_initial_source_url(local_url))
}

fn valid_initial_source_url(url: QUrl) -> Option<QUrl> {
    if url.is_empty() || !url.is_valid() {
        None
    } else {
        Some(url)
    }
}

fn validate_local_url(url: &QUrl) -> Result<(), StartupArgumentError> {
    let Some(local_file) = url.to_local_file() else {
        return Ok(());
    };

    let path = PathBuf::from(String::from(local_file));
    if path.as_os_str().is_empty() {
        return Ok(());
    }

    validate_local_file_path(&path)
}

fn validate_local_file_path(path: &Path) -> Result<(), StartupArgumentError> {
    match path.try_exists() {
        Ok(true) => Ok(()),
        Ok(false) => Err(StartupArgumentError {
            path: path.to_path_buf(),
            reason: MISSING_LOCAL_FILE_REASON.to_owned(),
        }),
        Err(error) => Err(StartupArgumentError {
            path: path.to_path_buf(),
            reason: error.to_string(),
        }),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::{
        env,
        ffi::OsString,
        fs,
        time::{SystemTime, UNIX_EPOCH},
    };

    fn startup_args(arguments: &[&str]) -> Vec<OsString> {
        std::iter::once(OsString::from("kiriview"))
            .chain(arguments.iter().map(OsString::from))
            .collect()
    }

    fn local_file_path(url: &QUrl) -> Option<String> {
        url.to_local_file().map(String::from)
    }

    fn test_dir(name: &str) -> PathBuf {
        let unique_suffix = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("system time should be after Unix epoch")
            .as_nanos();
        let path = env::temp_dir().join(format!(
            "kiriview-{name}-{}-{unique_suffix}",
            std::process::id()
        ));
        fs::create_dir(&path).expect("test directory should be created");
        path
    }

    fn existing_test_file(working_directory: &Path, name: &str) -> PathBuf {
        let path = working_directory.join(name);
        fs::write(&path, []).expect("test file should be created");
        path
    }

    #[test]
    fn relative_startup_path_becomes_local_file_url() {
        let working_directory = test_dir("relative-startup-path");
        let expected_path = existing_test_file(&working_directory, "archive-zip.cbt");

        let url = initial_source_url_from_args(
            startup_args(&["archive-zip.cbt"]),
            Some(&working_directory),
        )
        .expect("relative startup path should be accepted")
        .expect("relative startup path should produce a URL");

        assert_eq!(
            local_file_path(&url).map(PathBuf::from).as_deref(),
            Some(expected_path.as_path())
        );

        fs::remove_dir_all(working_directory).expect("test directory should be removed");
    }

    #[test]
    fn startup_path_after_separator_becomes_local_file_url() {
        let working_directory = test_dir("startup-path-after-separator");
        let expected_path = existing_test_file(&working_directory, "archive-zip.cbt");

        let url = initial_source_url_from_args(
            startup_args(&["--", "archive-zip.cbt"]),
            Some(&working_directory),
        )
        .expect("startup path after -- should be accepted")
        .expect("startup path after -- should produce a URL");

        assert_eq!(
            local_file_path(&url).map(PathBuf::from).as_deref(),
            Some(expected_path.as_path())
        );

        fs::remove_dir_all(working_directory).expect("test directory should be removed");
    }

    #[test]
    fn missing_relative_startup_path_reports_argument_error() {
        let working_directory = test_dir("missing-relative-startup-path");
        let expected_path = working_directory.join("non-exit-file");

        let error = initial_source_url_from_args(
            startup_args(&["non-exit-file"]),
            Some(&working_directory),
        )
        .expect_err("missing startup path should report an argument error");

        assert_eq!(error.path(), expected_path.as_path());
        assert_eq!(error.reason(), MISSING_LOCAL_FILE_REASON);

        fs::remove_dir_all(working_directory).expect("test directory should be removed");
    }

    #[test]
    fn missing_file_url_startup_argument_reports_argument_error() {
        let working_directory = test_dir("missing-file-url-startup-argument");
        let expected_path = working_directory.join("non-exit-file");
        let file_url =
            QUrl::from_local_file(&QString::from(expected_path.to_string_lossy().as_ref()));
        let args = vec![
            OsString::from("kiriview"),
            OsString::from(String::from(file_url.to_qstring())),
        ];

        let error = initial_source_url_from_args(args, Some(&working_directory))
            .expect_err("missing startup file URL should report an argument error");

        assert_eq!(error.path(), expected_path.as_path());
        assert_eq!(error.reason(), MISSING_LOCAL_FILE_REASON);

        fs::remove_dir_all(working_directory).expect("test directory should be removed");
    }

    #[test]
    fn startup_url_argument_keeps_its_scheme() {
        let url = initial_source_url_from_args(
            startup_args(&["smb://server/share/page.png"]),
            Some(Path::new("/home/user/books")),
        )
        .expect("startup URL should be accepted")
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
            .expect("separator-only startup arguments should be accepted")
            .is_none()
        );
    }
}
