// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntime.h"

#include "localization.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QtGlobal>

namespace {
void setupApplicationIdentity()
{
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.hnjae.KiriView"));
}

void setupDefaultQuickStyle()
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}
}

namespace KiriView {
void initializeApplicationRuntime()
{
    initializeLocalization();
    setupApplicationIdentity();
    setupDefaultQuickStyle();
}

void loadApplicationMainQml(QQmlApplicationEngine &engine, const QUrl &initialSourceUrl)
{
    setupLocalizedContext(engine);

    if (!initialSourceUrl.isEmpty()) {
        QVariantMap initialProperties;
        initialProperties.insert(QStringLiteral("initialSourceUrl"), initialSourceUrl);
        engine.setInitialProperties(initialProperties);
    }

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/io/github/hnjae/kiriview/src/qml/Main.qml")));
}
}
