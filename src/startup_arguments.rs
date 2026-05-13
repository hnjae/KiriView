// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

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

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum StartupSource {
    LocalFile(PathBuf),
    Url(String),
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

pub fn initial_source_from_args(
    args: impl IntoIterator<Item = impl AsRef<OsStr>>,
    working_directory: Option<&Path>,
) -> Result<Option<StartupSource>, StartupArgumentError> {
    let Some(argument) = args
        .into_iter()
        .skip(1)
        .find(|argument| argument.as_ref() != OsStr::new("--"))
    else {
        return Ok(None);
    };

    source_from_argument(argument.as_ref(), working_directory)
}

fn source_from_argument(
    argument: &OsStr,
    working_directory: Option<&Path>,
) -> Result<Option<StartupSource>, StartupArgumentError> {
    let argument = argument.to_string_lossy();
    if argument.is_empty() {
        return Ok(None);
    }

    if has_url_scheme(&argument) {
        if let Some(local_file_path) = local_file_path_from_file_url(&argument) {
            validate_non_empty_local_file_path(&local_file_path)?;
        }
        return Ok(Some(StartupSource::Url(argument.into_owned())));
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

    Ok(Some(StartupSource::LocalFile(absolute_path)))
}

fn has_url_scheme(argument: &str) -> bool {
    let Some((scheme, _)) = argument.split_once(':') else {
        return false;
    };
    let mut chars = scheme.chars();
    let Some(first) = chars.next() else {
        return false;
    };
    first.is_ascii_alphabetic()
        && chars.all(|character| {
            character.is_ascii_alphanumeric()
                || character == '+'
                || character == '-'
                || character == '.'
        })
}

fn local_file_path_from_file_url(url: &str) -> Option<PathBuf> {
    let (scheme, rest) = url.split_once(':')?;
    if !scheme.eq_ignore_ascii_case("file") {
        return None;
    }

    let path = if let Some(authority_and_path) = rest.strip_prefix("//") {
        let (authority, path) = authority_and_path
            .split_once('/')
            .map_or((authority_and_path, ""), |(authority, path)| {
                (authority, path)
            });
        if authority.is_empty() || authority.eq_ignore_ascii_case("localhost") {
            format!("/{path}")
        } else if path.is_empty() {
            format!("//{authority}")
        } else {
            format!("//{authority}/{path}")
        }
    } else {
        rest.to_owned()
    };

    Some(PathBuf::from(percent_decoded_path(&path)))
}

fn percent_decoded_path(path: &str) -> String {
    let bytes = path.as_bytes();
    let mut decoded = Vec::with_capacity(bytes.len());
    let mut index = 0;
    while index < bytes.len() {
        if bytes[index] == b'%'
            && index + 2 < bytes.len()
            && let Some(byte) = hex_byte(bytes[index + 1], bytes[index + 2])
        {
            decoded.push(byte);
            index += 3;
            continue;
        }

        decoded.push(bytes[index]);
        index += 1;
    }

    String::from_utf8_lossy(&decoded).into_owned()
}

fn hex_byte(high: u8, low: u8) -> Option<u8> {
    Some(hex_value(high)? * 16 + hex_value(low)?)
}

fn hex_value(value: u8) -> Option<u8> {
    match value {
        b'0'..=b'9' => Some(value - b'0'),
        b'a'..=b'f' => Some(value - b'a' + 10),
        b'A'..=b'F' => Some(value - b'A' + 10),
        _ => None,
    }
}

fn validate_non_empty_local_file_path(path: &Path) -> Result<(), StartupArgumentError> {
    if path.as_os_str().is_empty() {
        return Ok(());
    }
    validate_local_file_path(path)
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

    struct TestDirectory {
        path: PathBuf,
    }

    impl TestDirectory {
        fn new(name: &str) -> Self {
            let unique_suffix = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("system time should be after Unix epoch")
                .as_nanos();
            let path = env::temp_dir().join(format!(
                "kiriview-{name}-{}-{unique_suffix}",
                std::process::id()
            ));
            fs::create_dir(&path).expect("test directory should be created");
            Self { path }
        }

        fn path(&self) -> &Path {
            &self.path
        }
    }

    impl Drop for TestDirectory {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.path);
        }
    }

    fn existing_test_file(working_directory: &Path, name: &str) -> PathBuf {
        let path = working_directory.join(name);
        fs::write(&path, []).expect("test file should be created");
        path
    }

    #[test]
    fn relative_startup_path_becomes_local_file_source() {
        let working_directory = TestDirectory::new("relative-startup-path");
        let expected_path = existing_test_file(working_directory.path(), "archive-zip.cbt");

        let source = initial_source_from_args(
            startup_args(&["archive-zip.cbt"]),
            Some(working_directory.path()),
        )
        .expect("relative startup path should be accepted")
        .expect("relative startup path should produce a source");

        assert_eq!(source, StartupSource::LocalFile(expected_path));
    }

    #[test]
    fn startup_path_after_separator_becomes_local_file_source() {
        let working_directory = TestDirectory::new("startup-path-after-separator");
        let expected_path = existing_test_file(working_directory.path(), "archive-zip.cbt");

        let source = initial_source_from_args(
            startup_args(&["--", "archive-zip.cbt"]),
            Some(working_directory.path()),
        )
        .expect("startup path after -- should be accepted")
        .expect("startup path after -- should produce a source");

        assert_eq!(source, StartupSource::LocalFile(expected_path));
    }

    #[test]
    fn missing_relative_startup_path_reports_argument_error() {
        let working_directory = TestDirectory::new("missing-relative-startup-path");
        let expected_path = working_directory.path().join("non-exit-file");

        let error = initial_source_from_args(
            startup_args(&["non-exit-file"]),
            Some(working_directory.path()),
        )
        .expect_err("missing startup path should report an argument error");

        assert_eq!(error.path(), expected_path.as_path());
        assert_eq!(error.reason(), MISSING_LOCAL_FILE_REASON);
    }

    #[test]
    fn missing_file_url_startup_argument_reports_argument_error() {
        let working_directory = TestDirectory::new("missing-file-url-startup-argument");
        let expected_path = working_directory.path().join("non-exit-file");
        let file_url = format!("file://{}", expected_path.display());
        let args = vec![OsString::from("kiriview"), OsString::from(file_url)];

        let error = initial_source_from_args(args, Some(working_directory.path()))
            .expect_err("missing startup file URL should report an argument error");

        assert_eq!(error.path(), expected_path.as_path());
        assert_eq!(error.reason(), MISSING_LOCAL_FILE_REASON);
    }

    #[test]
    fn startup_url_argument_keeps_its_scheme() {
        let source = initial_source_from_args(
            startup_args(&["smb://server/share/page.png"]),
            Some(Path::new("/home/user/books")),
        )
        .expect("startup URL should be accepted")
        .expect("startup URL should produce a source");

        assert_eq!(
            source,
            StartupSource::Url("smb://server/share/page.png".to_owned())
        );
    }

    #[test]
    fn percent_encoded_file_url_validates_decoded_local_path() {
        let working_directory = TestDirectory::new("encoded-file-url");
        let expected_path = existing_test_file(working_directory.path(), "archive zip.cbt");
        let file_url = format!(
            "file://{}",
            expected_path.to_string_lossy().replace(' ', "%20")
        );

        let source = initial_source_from_args(
            [OsString::from("kiriview"), OsString::from(file_url.clone())],
            Some(working_directory.path()),
        )
        .expect("file URL should be accepted")
        .expect("file URL should produce a source");

        assert_eq!(source, StartupSource::Url(file_url));
    }

    #[test]
    fn startup_separator_without_path_produces_no_source() {
        assert!(
            initial_source_from_args(startup_args(&["--"]), Some(Path::new("/home/user/books")))
                .expect("separator-only startup arguments should be accepted")
                .is_none()
        );
    }
}
