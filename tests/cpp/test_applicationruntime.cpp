// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>

namespace {
KiriView::ApplicationInitialSource startupSource(
    KiriView::ApplicationInitialSourceKind kind, const QString &text = {})
{
    return KiriView::ApplicationInitialSource { kind, text };
}
}

class TestApplicationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startupSourceUrlIsEmptyWithoutSource();
    void startupSourceUrlUsesLocalFilePath();
    void startupSourceUrlUsesUrlText();
    void startupSourceUrlRejectsEmptyUrlText();
};

void TestApplicationRuntime::startupSourceUrlIsEmptyWithoutSource()
{
    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationInitialSourceKind::None));

    QVERIFY(url.isEmpty());
}

void TestApplicationRuntime::startupSourceUrlUsesLocalFilePath()
{
    const QString path = QStringLiteral("/tmp/kiriview/image.png");

    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationInitialSourceKind::LocalFilePath, path));

    QVERIFY(url.isValid());
    QVERIFY(url.isLocalFile());
    QCOMPARE(url.toLocalFile(), path);
}

void TestApplicationRuntime::startupSourceUrlUsesUrlText()
{
    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationInitialSourceKind::UrlText,
            QStringLiteral("https://example.invalid/image.png")));

    QVERIFY(url.isValid());
    QCOMPARE(url, QUrl(QStringLiteral("https://example.invalid/image.png")));
}

void TestApplicationRuntime::startupSourceUrlRejectsEmptyUrlText()
{
    const QUrl url = KiriView::initialSourceUrlFromStartupSource(
        startupSource(KiriView::ApplicationInitialSourceKind::UrlText));

    QVERIFY(url.isEmpty());
}

QTEST_GUILESS_MAIN(TestApplicationRuntime)

#include "test_applicationruntime.moc"
