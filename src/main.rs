// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{QMap, QMapPair_QString_QVariant, QQmlApplicationEngine, QString, QUrl, QVariant};
use cxx_qt_lib_extras::QApplication;
use std::{env, path::Path, process};

fn initial_source_url() -> Result<Option<QUrl>, kiriview::StartupArgumentError> {
    let working_directory = env::current_dir().ok();
    let source = kiriview::initial_source_from_args(env::args_os(), working_directory.as_deref())?;
    Ok(source.and_then(startup_source_url))
}

fn startup_source_url(source: kiriview::StartupSource) -> Option<QUrl> {
    match source {
        kiriview::StartupSource::LocalFile(path) => valid_initial_source_url(local_file_url(&path)),
        kiriview::StartupSource::Url(url) => {
            valid_initial_source_url(QUrl::from(&QString::from(url.as_str())))
        }
    }
}

fn local_file_url(path: &Path) -> QUrl {
    QUrl::from_local_file(&QString::from(path.to_string_lossy().as_ref()))
}

fn valid_initial_source_url(url: QUrl) -> Option<QUrl> {
    if url.is_empty() || !url.is_valid() {
        None
    } else {
        Some(url)
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
        kiriview::setup_localized_context(engine.as_mut());

        if let Some(url) = initial_source_url.as_ref() {
            let mut initial_properties = QMap::<QMapPair_QString_QVariant>::default();
            initial_properties
                .insert_clone(&QString::from("initialSourceUrl"), &QVariant::from(url));
            engine.as_mut().set_initial_properties(&initial_properties);
        }

        engine.as_mut().load(&QUrl::from(
            "qrc:/qt/qml/io/github/hnjae/kiriview/src/qml/Main.qml",
        ));
    }

    if let Some(app) = app.as_mut() {
        app.exec();
    }
}
