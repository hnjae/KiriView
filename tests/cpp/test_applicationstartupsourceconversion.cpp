// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/applicationstartupsourceconversion.h"

#include "application/applicationstartupsource.h"
#include "kiriview/src/policy/applicationruntime.cxx.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <string>

namespace {
KiriView::ApplicationStartupSource bridgeStartupSource(
    KiriView::ApplicationStartupSourceKind kind, const QString &text = {})
{
    const QByteArray utf8Text = text.toUtf8();
    return KiriView::ApplicationStartupSource {
        kind,
        rust::String(std::string(utf8Text.constData(), static_cast<std::size_t>(utf8Text.size()))),
    };
}
}

class TestApplicationStartupSourceConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bridgeStartupSourceProjectsAbsentSource();
    void bridgeStartupSourceProjectsLocalFilePath();
    void bridgeStartupSourceProjectsUrlText();
};

void TestApplicationStartupSourceConversion::bridgeStartupSourceProjectsAbsentSource()
{
    const KiriView::ApplicationInitialSource source = KiriView::applicationInitialSourceFromBridge(
        bridgeStartupSource(KiriView::ApplicationStartupSourceKind::None));

    QVERIFY(source.kind == KiriView::ApplicationInitialSourceKind::None);
    QVERIFY(source.text.isEmpty());
}

void TestApplicationStartupSourceConversion::bridgeStartupSourceProjectsLocalFilePath()
{
    const QString path = QStringLiteral("/tmp/kiriview/image.png");

    const KiriView::ApplicationInitialSource source = KiriView::applicationInitialSourceFromBridge(
        bridgeStartupSource(KiriView::ApplicationStartupSourceKind::LocalFilePath, path));

    QVERIFY(source.kind == KiriView::ApplicationInitialSourceKind::LocalFilePath);
    QCOMPARE(source.text, path);
}

void TestApplicationStartupSourceConversion::bridgeStartupSourceProjectsUrlText()
{
    const QString url = QStringLiteral("https://example.invalid/image.png");

    const KiriView::ApplicationInitialSource source = KiriView::applicationInitialSourceFromBridge(
        bridgeStartupSource(KiriView::ApplicationStartupSourceKind::UrlText, url));

    QVERIFY(source.kind == KiriView::ApplicationInitialSourceKind::UrlText);
    QCOMPARE(source.text, url);
}

QTEST_GUILESS_MAIN(TestApplicationStartupSourceConversion)

#include "test_applicationstartupsourceconversion.moc"
