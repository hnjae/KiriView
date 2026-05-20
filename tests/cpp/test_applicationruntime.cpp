// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationruntime.h"

#include "kiriview/src/policy/applicationruntime.cxx.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>
#include <string>

namespace {
KiriView::ApplicationStartupSource startupSource(
    KiriView::ApplicationStartupSourceKind kind, const QString &text = {})
{
    const QByteArray utf8Text = text.toUtf8();
    return KiriView::ApplicationStartupSource {
        kind,
        rust::String(std::string(utf8Text.constData(), static_cast<std::size_t>(utf8Text.size()))),
    };
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

QTEST_GUILESS_MAIN(TestApplicationRuntime)

#include "test_applicationruntime.moc"
