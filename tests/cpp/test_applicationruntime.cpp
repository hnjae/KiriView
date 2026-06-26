// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"
#include "application/applicationstartupsource.h"
#include "kiriview/src/policy/applicationruntime.cxx.h"

#include <QByteArray>
#include <QLoggingCategory>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <string>

namespace {
kiriview::ApplicationStartupSource startupSource(
    kiriview::ApplicationStartupSourceKind kind, const QString& text = {}, bool verbose = false)
{
    const QByteArray utf8Text = text.toUtf8();
    return kiriview::ApplicationStartupSource {
        kind,
        rust::String(std::string(utf8Text.constData(), static_cast<std::size_t>(utf8Text.size()))),
        verbose,
    };
}

bool categoryDebugEnabled(const QString& name)
{
    const QByteArray utf8Name = name.toUtf8();
    QLoggingCategory category(utf8Name.constData(), QtWarningMsg);
    return category.isDebugEnabled();
}

QStringList diagnosticCategoryNames()
{
    return {
        QStringLiteral("org.hnjae.kiriview.decode"),
        QStringLiteral("org.hnjae.kiriview.navigation"),
        QStringLiteral("org.hnjae.kiriview.predecode"),
        QStringLiteral("org.hnjae.kiriview.thumbnail"),
        QStringLiteral("org.hnjae.kiriview.display.provider"),
        QStringLiteral("org.hnjae.kiriview.animation"),
    };
}
}

class TestApplicationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cleanup();
    void startupSourceUrlIsEmptyWithoutSource();
    void startupSourceUrlUsesLocalFilePath();
    void startupSourceUrlUsesUrlText();
    void startupSourceUrlRejectsEmptyUrlText();
    void startupSourceCarriesVerboseMode();
    void runtimeDiagnosticsStayDisabledWithoutVerboseStartup();
    void runtimeDiagnosticsEnableVerboseStartupCategories();
    void registersDisplayImageProvider();
};

void TestApplicationRuntime::cleanup() { QLoggingCategory::setFilterRules(QString()); }

void TestApplicationRuntime::startupSourceUrlIsEmptyWithoutSource()
{
    const QUrl url = kiriview::initialSourceUrlFromStartupSource(
        startupSource(kiriview::ApplicationStartupSourceKind::None));

    QVERIFY(url.isEmpty());
}

void TestApplicationRuntime::startupSourceUrlUsesLocalFilePath()
{
    const QString path = QStringLiteral("/tmp/kiriview/image.png");

    const QUrl url = kiriview::initialSourceUrlFromStartupSource(
        startupSource(kiriview::ApplicationStartupSourceKind::LocalFilePath, path));

    QVERIFY(url.isValid());
    QVERIFY(url.isLocalFile());
    QCOMPARE(url.toLocalFile(), path);
}

void TestApplicationRuntime::startupSourceUrlUsesUrlText()
{
    const QUrl url = kiriview::initialSourceUrlFromStartupSource(
        startupSource(kiriview::ApplicationStartupSourceKind::UrlText,
            QStringLiteral("https://example.invalid/image.png")));

    QVERIFY(url.isValid());
    QCOMPARE(url, QUrl(QStringLiteral("https://example.invalid/image.png")));
}

void TestApplicationRuntime::startupSourceUrlRejectsEmptyUrlText()
{
    const QUrl url = kiriview::initialSourceUrlFromStartupSource(
        startupSource(kiriview::ApplicationStartupSourceKind::UrlText));

    QVERIFY(url.isEmpty());
}

void TestApplicationRuntime::startupSourceCarriesVerboseMode()
{
    const kiriview::ApplicationStartupSource source
        = startupSource(kiriview::ApplicationStartupSourceKind::None, {}, true);

    QVERIFY(source.verbose);
}

void TestApplicationRuntime::runtimeDiagnosticsStayDisabledWithoutVerboseStartup()
{
    QLoggingCategory::setFilterRules(QStringLiteral("org.hnjae.kiriview.*.debug=false"));

    kiriview::configureApplicationRuntimeDiagnostics(
        startupSource(kiriview::ApplicationStartupSourceKind::None));

    for (const QString& categoryName : diagnosticCategoryNames()) {
        QVERIFY2(!categoryDebugEnabled(categoryName), qPrintable(categoryName));
    }
}

void TestApplicationRuntime::runtimeDiagnosticsEnableVerboseStartupCategories()
{
    QLoggingCategory::setFilterRules(QStringLiteral("org.hnjae.kiriview.*.debug=false"));

    kiriview::configureApplicationRuntimeDiagnostics(
        startupSource(kiriview::ApplicationStartupSourceKind::None, {}, true));

    for (const QString& categoryName : diagnosticCategoryNames()) {
        QVERIFY2(categoryDebugEnabled(categoryName), qPrintable(categoryName));
    }
}

void TestApplicationRuntime::registersDisplayImageProvider()
{
    QQmlEngine engine;

    kiriview::registerApplicationImageProviders(engine);

    QVERIFY(engine.imageProvider(QStringLiteral("kiriview-thumbnails")) != nullptr);
    QVERIFY(engine.imageProvider(QStringLiteral("kiriview-images")) != nullptr);
}

QTEST_GUILESS_MAIN(TestApplicationRuntime)

#include "test_applicationruntime.moc"
