// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{QQmlApplicationEngine, QUrl};
use std::pin::Pin;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qqmlapplicationengine.h");
        include!("cxx-qt-lib/qurl.h");
        #[namespace = ""]
        type QQmlApplicationEngine = cxx_qt_lib::QQmlApplicationEngine;
        #[namespace = ""]
        type QUrl = cxx_qt_lib::QUrl;

        include!("applicationruntime.h");

        #[cxx_name = "initializeApplicationRuntime"]
        fn initialize_application_runtime();

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

pub fn load_application_main_qml(
    engine: Pin<&mut QQmlApplicationEngine>,
    initial_source_url: &QUrl,
) {
    ffi::load_application_main_qml(engine, initial_source_url);
}
