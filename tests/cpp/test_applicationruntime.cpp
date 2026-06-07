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
KiriView::ApplicationStartupSource startupSource(
    KiriView::ApplicationStartupSourceKind kind, const QString &text = {}, bool verbose = false)
{
    const QByteArray utf8Text = text.toUtf8();
    return KiriView::ApplicationStartupSource {
        kind,
        rust::String(std::string(utf8Text.constData(), static_cast<std::size_t>(utf8Text.size()))),
        verbose,
    };
}

bool categoryDebugEnabled(const QString &name)
{
    const QByteArray utf8Name = name.toUtf8();
    QLoggingCategory category(utf8Name.constData(), QtWarningMsg);
    return category.isDebugEnabled();
}

QStringList diagnosticCategoryNames()
{
    return {
        QStringLiteral("io.github.hnjae.kiriview.decode"),
        QStringLiteral("io.github.hnjae.kiriview.navigation"),
        QStringLiteral("io.github.hnjae.kiriview.predecode"),
        QStringLiteral("io.github.hnjae.kiriview.thumbnail"),
        QStringLiteral("io.github.hnjae.kiriview.display.provider"),
        QStringLiteral("io.github.hnjae.kiriview.animation"),
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
    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationStartupSourceKind::None));

    QVERIFY(url.isEmpty());
}

void TestApplicationRuntime::startupSourceUrlUsesLocalFilePath()
{
    const QString path = QStringLiteral("/tmp/kiriview/image.png");

    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationStartupSourceKind::LocalFilePath, path));

    QVERIFY(url.isValid());
    QVERIFY(url.isLocalFile());
    QCOMPARE(url.toLocalFile(), path);
}

void TestApplicationRuntime::startupSourceUrlUsesUrlText()
{
    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationStartupSourceKind::UrlText,
            QStringLiteral("https://example.invalid/image.png")));

    QVERIFY(url.isValid());
    QCOMPARE(url, QUrl(QStringLiteral("https://example.invalid/image.png")));
}

void TestApplicationRuntime::startupSourceUrlRejectsEmptyUrlText()
{
    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationStartupSourceKind::UrlText));

    QVERIFY(url.isEmpty());
}

void TestApplicationRuntime::startupSourceCarriesVerboseMode()
{
    const KiriView::ApplicationStartupSource source
        = startupSource(KiriView::ApplicationStartupSourceKind::None, {}, true);

    QVERIFY(source.verbose);
}

void TestApplicationRuntime::runtimeDiagnosticsStayDisabledWithoutVerboseStartup()
{
    QLoggingCategory::setFilterRules(QStringLiteral("io.github.hnjae.kiriview.*.debug=false"));

    KiriView::configureApplicationRuntimeDiagnostics(
        startupSource(KiriView::ApplicationStartupSourceKind::None));

    for (const QString &categoryName : diagnosticCategoryNames()) {
        QVERIFY2(!categoryDebugEnabled(categoryName), qPrintable(categoryName));
    }
}

void TestApplicationRuntime::runtimeDiagnosticsEnableVerboseStartupCategories()
{
    QLoggingCategory::setFilterRules(QStringLiteral("io.github.hnjae.kiriview.*.debug=false"));

    KiriView::configureApplicationRuntimeDiagnostics(
        startupSource(KiriView::ApplicationStartupSourceKind::None, {}, true));

    for (const QString &categoryName : diagnosticCategoryNames()) {
        QVERIFY2(categoryDebugEnabled(categoryName), qPrintable(categoryName));
    }
}

void TestApplicationRuntime::registersDisplayImageProvider()
{
    QQmlEngine engine;

    KiriView::registerApplicationImageProviders(engine);

    QVERIFY(engine.imageProvider(QStringLiteral("kiriview-thumbnails")) != nullptr);
    QVERIFY(engine.imageProvider(QStringLiteral("kiriview-images")) != nullptr);
}

QTEST_GUILESS_MAIN(TestApplicationRuntime)

#include "test_applicationruntime.moc"
