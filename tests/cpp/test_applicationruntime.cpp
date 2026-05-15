// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestApplicationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialSourceUrlUsesLocalFilePath();
    void initialSourceUrlUsesUrlText();
    void initialSourceUrlRejectsEmptyUrlText();
};

void TestApplicationRuntime::initialSourceUrlUsesLocalFilePath()
{
    const QString path = QStringLiteral("/tmp/kiriview/image.png");

    const QUrl url = KiriView::initialSourceUrlFromLocalFilePath(path);

    QVERIFY(url.isValid());
    QVERIFY(url.isLocalFile());
    QCOMPARE(url.toLocalFile(), path);
}

void TestApplicationRuntime::initialSourceUrlUsesUrlText()
{
    const QUrl url = KiriView::initialSourceUrlFromUrlText(
        QStringLiteral("https://example.invalid/image.png"));

    QVERIFY(url.isValid());
    QCOMPARE(url, QUrl(QStringLiteral("https://example.invalid/image.png")));
}

void TestApplicationRuntime::initialSourceUrlRejectsEmptyUrlText()
{
    const QUrl url = KiriView::initialSourceUrlFromUrlText(QString());

    QVERIFY(url.isEmpty());
}

QTEST_GUILESS_MAIN(TestApplicationRuntime)

#include "test_applicationruntime.moc"
