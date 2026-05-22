// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::startup_arguments::StartupSource;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    enum ApplicationStartupSourceKind {
        None,
        LocalFilePath,
        UrlText,
    }

    struct ApplicationStartupSource {
        kind: ApplicationStartupSourceKind,
        text: String,
    }

    unsafe extern "C++" {
        include!("application/applicationruntimebridge.h");

        #[cxx_name = "runApplicationFromBridge"]
        fn run_application_from_bridge(startup_source: &ApplicationStartupSource) -> i32;
    }
}

pub use ffi::{ApplicationStartupSource, ApplicationStartupSourceKind};

pub fn application_startup_source(source: Option<StartupSource>) -> ApplicationStartupSource {
    match source {
        Some(StartupSource::LocalFile(path)) => ApplicationStartupSource {
            kind: ApplicationStartupSourceKind::LocalFilePath,
            text: path.to_string_lossy().into_owned(),
        },
        Some(StartupSource::Url(url)) => ApplicationStartupSource {
            kind: ApplicationStartupSourceKind::UrlText,
            text: url,
        },
        None => ApplicationStartupSource {
            kind: ApplicationStartupSourceKind::None,
            text: String::new(),
        },
    }
}

pub fn run_application(startup_source: &ApplicationStartupSource) -> i32 {
    ffi::run_application_from_bridge(startup_source)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    #[test]
    fn startup_source_projects_absent_source() {
        let source = application_startup_source(None);

        assert!(source.kind == ApplicationStartupSourceKind::None);
        assert!(source.text.is_empty());
    }

    #[test]
    fn startup_source_projects_local_file_path() {
        let source = application_startup_source(Some(StartupSource::LocalFile(PathBuf::from(
            "/tmp/kiriview/image.png",
        ))));

        assert!(source.kind == ApplicationStartupSourceKind::LocalFilePath);
        assert_eq!(source.text, "/tmp/kiriview/image.png");
    }

    #[test]
    fn startup_source_projects_url_text() {
        let source = application_startup_source(Some(StartupSource::Url(String::from(
            "https://example.invalid/image.png",
        ))));

        assert!(source.kind == ApplicationStartupSourceKind::UrlText);
        assert_eq!(source.text, "https://example.invalid/image.png");
    }
}
