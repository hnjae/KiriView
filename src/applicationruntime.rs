// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{QQmlApplicationEngine, QString, QUrl};
use std::pin::Pin;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qqmlapplicationengine.h");
        include!("cxx-qt-lib/qstring.h");
        include!("cxx-qt-lib/qurl.h");
        #[namespace = ""]
        type QQmlApplicationEngine = cxx_qt_lib::QQmlApplicationEngine;
        #[namespace = ""]
        type QString = cxx_qt_lib::QString;
        #[namespace = ""]
        type QUrl = cxx_qt_lib::QUrl;

        include!("applicationruntime.h");

        #[cxx_name = "initializeApplicationRuntime"]
        fn initialize_application_runtime();

        #[cxx_name = "initialSourceUrlFromLocalFilePath"]
        fn initial_source_url_from_local_file_path(path: &QString) -> QUrl;

        #[cxx_name = "initialSourceUrlFromUrlText"]
        fn initial_source_url_from_url_text(url_text: &QString) -> QUrl;

        #[cxx_name = "loadApplicationMainQml"]
        fn load_application_main_qml(
            engine: Pin<&mut QQmlApplicationEngine>,
            initial_source_url: &QUrl,
        );
    }
}

pub fn initialize_application_runtime() {
    ffi::initialize_application_runtime();
}

pub fn initial_source_url_from_local_file_path(path: &QString) -> QUrl {
    ffi::initial_source_url_from_local_file_path(path)
}

pub fn initial_source_url_from_url_text(url_text: &QString) -> QUrl {
    ffi::initial_source_url_from_url_text(url_text)
}

pub fn load_application_main_qml(
    engine: Pin<&mut QQmlApplicationEngine>,
    initial_source_url: &QUrl,
) {
    ffi::load_application_main_qml(engine, initial_source_url);
}
