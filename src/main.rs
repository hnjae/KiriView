// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use cxx_qt_lib::{
    QGuiApplication, QMap, QMapPair_QString_QVariant, QQmlApplicationEngine, QQuickStyle, QString,
    QUrl, QVariant,
};
use cxx_qt_lib_extras::QApplication;
use std::env;

fn initial_source_url() -> Option<QUrl> {
    let working_directory = env::current_dir().ok();
    kiriview::initial_source_url_from_args(env::args_os(), working_directory.as_deref())
}

fn main() {
    kiriview::initialize_rust_modules();

    let mut app = QApplication::new();
    let initial_source_url = initial_source_url();

    QGuiApplication::set_desktop_file_name(&QString::from("io.github.hnjae.KiriView"));

    if env::var("QT_QUICK_CONTROLS_STYLE").is_err() {
        QQuickStyle::set_style(&QString::from("org.kde.desktop"));
    }

    let mut engine = QQmlApplicationEngine::new();
    if let Some(mut engine) = engine.as_mut() {
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
