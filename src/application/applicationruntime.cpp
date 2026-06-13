// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntime.h"

#include "applicationdiagnostics.h"
#include "applicationstartupsource.h"
#include "kiriview/src/policy/applicationruntime.cxx.h"
#include "localization/localization.h"
#include "rendering/displayimagestore.h"
#include "session/thumbnailimagestore.h"

#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QtGlobal>
#include <array>

namespace {
void setupApplicationIdentity()
{
    QGuiApplication::setDesktopFileName(QStringLiteral("org.hnjae.kiriview"));
}

void setupDefaultQuickStyle()
{
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}
}

namespace kiriview {
void initializeApplicationRuntime()
{
    initializeLocalization();
    setupApplicationIdentity();
    setupDefaultQuickStyle();
}

void configureApplicationRuntimeDiagnostics(const ApplicationStartupSource &startupSource)
{
    configureApplicationDiagnosticLogging(startupSource.verbose);
}

void registerApplicationImageProviders(QQmlEngine &engine)
{
    engine.addImageProvider(QStringLiteral("kiriview-thumbnails"),
        new ThumbnailImageProvider(sharedThumbnailImageStore()));
    engine.addImageProvider(
        QStringLiteral("kiriview-images"), new DisplayImageProvider(sharedDisplayImageStore()));
}

void loadApplicationMainQml(
    QQmlApplicationEngine &engine, const ApplicationStartupSource &startupSource)
{
    setupLocalizedContext(engine);
    registerApplicationImageProviders(engine);

    const QUrl initialSourceUrl = initialSourceUrlFromStartupSource(startupSource);
    if (!initialSourceUrl.isEmpty()) {
        QVariantMap initialProperties;
        initialProperties.insert(QStringLiteral("initialSourceUrl"), initialSourceUrl);
        engine.setInitialProperties(initialProperties);
    }

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/org/hnjae/kiriview/src/qml/Main.qml")));
}

int runApplication(const ApplicationStartupSource &startupSource)
{
    std::array<char, 9> applicationName { 'k', 'i', 'r', 'i', 'v', 'i', 'e', 'w', '\0' };
    char *arguments[] = { applicationName.data() };
    int argumentCount = 1;

    QApplication application(argumentCount, arguments);
    initializeApplicationRuntime();
    configureApplicationRuntimeDiagnostics(startupSource);

    QQmlApplicationEngine engine;
    loadApplicationMainQml(engine, startupSource);

    return application.exec();
}
}
