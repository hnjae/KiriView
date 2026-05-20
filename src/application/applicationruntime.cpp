// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntime.h"

#include "kiriview/src/policy/applicationruntime.cxx.h"
#include "localization.h"

#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QtGlobal>
#include <array>

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

QUrl validInitialSourceUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        return QUrl();
    }

    return url;
}

QString rustStringToQString(const rust::String &text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}
}

namespace KiriView {
void initializeApplicationRuntime()
{
    initializeLocalization();
    setupApplicationIdentity();
    setupDefaultQuickStyle();
}

QUrl initialSourceUrlFromStartupSource(const ApplicationStartupSource &source)
{
    const QString text = rustStringToQString(source.text);
    switch (source.kind) {
    case ApplicationStartupSourceKind::None:
        return QUrl();
    case ApplicationStartupSourceKind::LocalFilePath:
        return validInitialSourceUrl(QUrl::fromLocalFile(text));
    case ApplicationStartupSourceKind::UrlText:
        return validInitialSourceUrl(QUrl(text));
    }

    return QUrl();
}

void loadApplicationMainQml(
    QQmlApplicationEngine &engine, const ApplicationStartupSource &startupSource)
{
    setupLocalizedContext(engine);

    const QUrl initialSourceUrl = initialSourceUrlFromStartupSource(startupSource);
    if (!initialSourceUrl.isEmpty()) {
        QVariantMap initialProperties;
        initialProperties.insert(QStringLiteral("initialSourceUrl"), initialSourceUrl);
        engine.setInitialProperties(initialProperties);
    }

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/io/github/hnjae/kiriview/src/qml/Main.qml")));
}

int runApplication(const ApplicationStartupSource &startupSource)
{
    std::array<char, 9> applicationName { 'k', 'i', 'r', 'i', 'v', 'i', 'e', 'w', '\0' };
    char *arguments[] = { applicationName.data() };
    int argumentCount = 1;

    QApplication application(argumentCount, arguments);
    initializeApplicationRuntime();

    QQmlApplicationEngine engine;
    loadApplicationMainQml(engine, startupSource);

    return application.exec();
}
}
