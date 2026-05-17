// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::QQmlApplicationEngine;
use cxx_qt_lib_extras::QApplication;
use std::{env, process};

fn startup_source() -> Result<kiriview::ApplicationStartupSource, kiriview::StartupArgumentError> {
    let working_directory = env::current_dir().ok();
    let source = kiriview::initial_source_from_args(env::args_os(), working_directory.as_deref())?;
    Ok(kiriview::application_startup_source(source))
}

fn main() {
    let startup_source = match startup_source() {
        Ok(source) => source,
        Err(error) => {
            eprintln!("KiriView: {error}");
            process::exit(kiriview::STARTUP_ARGUMENT_ERROR_EXIT_CODE);
        }
    };

    kiriview::initialize_rust_modules();

    let mut app = QApplication::new();
    kiriview::initialize_application_runtime();

    let mut engine = QQmlApplicationEngine::new();
    if let Some(mut engine) = engine.as_mut() {
        kiriview::load_application_main_qml(engine.as_mut(), &startup_source);
    }

    if let Some(app) = app.as_mut() {
        app.exec();
    }
}
