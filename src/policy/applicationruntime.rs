// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::startup_arguments::{StartupOptions, StartupSource};

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
        verbose: bool,
    }

    unsafe extern "C++" {
        include!("application/applicationruntime.h");

        #[cxx_name = "runApplication"]
        fn run_application_cpp(startup_source: &ApplicationStartupSource) -> i32;
    }
}

pub use ffi::{ApplicationStartupSource, ApplicationStartupSourceKind};

pub fn application_startup_source(options: StartupOptions) -> ApplicationStartupSource {
    application_startup_source_from_parts(options.source, options.verbose)
}

fn application_startup_source_from_parts(
    source: Option<StartupSource>,
    verbose: bool,
) -> ApplicationStartupSource {
    let (kind, text) = match source {
        Some(StartupSource::LocalFile(path)) => (
            ApplicationStartupSourceKind::LocalFilePath,
            path.to_string_lossy().into_owned(),
        ),
        Some(StartupSource::Url(url)) => (ApplicationStartupSourceKind::UrlText, url),
        None => (ApplicationStartupSourceKind::None, String::new()),
    };

    ApplicationStartupSource {
        kind,
        text,
        verbose,
    }
}

pub fn run_application(startup_source: &ApplicationStartupSource) -> i32 {
    ffi::run_application_cpp(startup_source)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    fn startup_options(source: Option<StartupSource>, verbose: bool) -> StartupOptions {
        StartupOptions { source, verbose }
    }

    #[test]
    fn startup_source_projects_absent_source() {
        let source = application_startup_source(startup_options(None, false));

        assert!(source.kind == ApplicationStartupSourceKind::None);
        assert!(source.text.is_empty());
        assert!(!source.verbose);
    }

    #[test]
    fn startup_source_projects_verbose_mode() {
        let source = application_startup_source(startup_options(None, true));

        assert!(source.kind == ApplicationStartupSourceKind::None);
        assert!(source.text.is_empty());
        assert!(source.verbose);
    }

    #[test]
    fn startup_source_projects_local_file_path() {
        let source = application_startup_source(startup_options(
            Some(StartupSource::LocalFile(PathBuf::from(
                "/tmp/kiriview/image.png",
            ))),
            false,
        ));

        assert!(source.kind == ApplicationStartupSourceKind::LocalFilePath);
        assert_eq!(source.text, "/tmp/kiriview/image.png");
        assert!(!source.verbose);
    }

    #[test]
    fn startup_source_projects_url_text() {
        let source = application_startup_source(startup_options(
            Some(StartupSource::Url(String::from(
                "https://example.invalid/image.png",
            ))),
            false,
        ));

        assert!(source.kind == ApplicationStartupSourceKind::UrlText);
        assert_eq!(source.text, "https://example.invalid/image.png");
        assert!(!source.verbose);
    }
}
