// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{QQmlApplicationEngine, QString, QUrl};
use cxx_qt_lib_extras::QApplication;
use std::{env, process};

fn initial_source_url() -> Result<QUrl, kiriview::StartupArgumentError> {
    let working_directory = env::current_dir().ok();
    let source = kiriview::initial_source_from_args(env::args_os(), working_directory.as_deref())?;
    Ok(source.map(startup_source_url).unwrap_or_default())
}

fn startup_source_url(source: kiriview::StartupSource) -> QUrl {
    match source {
        kiriview::StartupSource::LocalFile(path) => {
            let path = QString::from(path.to_string_lossy().as_ref());
            kiriview::initial_source_url_from_local_file_path(&path)
        }
        kiriview::StartupSource::Url(url) => {
            kiriview::initial_source_url_from_url_text(&QString::from(url.as_str()))
        }
    }
}

fn main() {
    let initial_source_url = match initial_source_url() {
        Ok(url) => url,
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
        kiriview::load_application_main_qml(engine.as_mut(), &initial_source_url);
    }

    if let Some(app) = app.as_mut() {
        app.exec();
    }
}
