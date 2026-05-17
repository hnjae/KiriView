// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::startup_arguments::StartupSource;
use cxx_qt_lib::QQmlApplicationEngine;
use std::pin::Pin;

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
        include!("cxx-qt-lib/qqmlapplicationengine.h");
        #[namespace = ""]
        type QQmlApplicationEngine = cxx_qt_lib::QQmlApplicationEngine;

        include!("applicationruntime.h");

        #[cxx_name = "initializeApplicationRuntime"]
        fn initialize_application_runtime();

        #[cxx_name = "loadApplicationMainQml"]
        fn load_application_main_qml(
            engine: Pin<&mut QQmlApplicationEngine>,
            startup_source: &ApplicationStartupSource,
        );
    }
}

pub use ffi::{ApplicationStartupSource, ApplicationStartupSourceKind};

pub fn initialize_application_runtime() {
    ffi::initialize_application_runtime();
}

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

pub fn load_application_main_qml(
    engine: Pin<&mut QQmlApplicationEngine>,
    startup_source: &ApplicationStartupSource,
) {
    ffi::load_application_main_qml(engine, startup_source);
}
