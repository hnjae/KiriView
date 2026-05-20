// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecoderequest.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestImageDecodeRequest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void matchesUsesIdAndNormalizedImageUrl();
};

void TestImageDecodeRequest::matchesUsesIdAndNormalizedImageUrl()
{
    const KiriView::ImageDecodeRequest request = KiriView::ImageDecodeRequest::fromUrl(
        42, QUrl(QStringLiteral("file:///images/./page.png")));

    QVERIFY(request.matches(42, QUrl(QStringLiteral("file:///images/page.png"))));
    QVERIFY(request.matches(KiriView::ImageDecodeRequest::fromUrl(
        42, QUrl(QStringLiteral("file:///images/page.png")))));
    QVERIFY(!request.matches(43, QUrl(QStringLiteral("file:///images/page.png"))));
    QVERIFY(!request.matches(42, QUrl(QStringLiteral("file:///images/other.png"))));
}

QTEST_GUILESS_MAIN(TestImageDecodeRequest)

#include "test_imagedecoderequest.moc"
