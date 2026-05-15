// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::QQmlApplicationEngine;
use std::pin::Pin;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qqmlapplicationengine.h");
        #[namespace = ""]
        type QQmlApplicationEngine = cxx_qt_lib::QQmlApplicationEngine;

        include!("applicationruntime.h");
        include!("localization.h");

        #[cxx_name = "initializeApplicationRuntime"]
        fn initialize_application_runtime();

        #[cxx_name = "setupLocalizedContext"]
        fn setup_localized_context(engine: Pin<&mut QQmlApplicationEngine>);
    }
}

pub fn initialize_application_runtime() {
    ffi::initialize_application_runtime();
}

pub fn setup_localized_context(engine: Pin<&mut QQmlApplicationEngine>) {
    ffi::setup_localized_context(engine);
}
