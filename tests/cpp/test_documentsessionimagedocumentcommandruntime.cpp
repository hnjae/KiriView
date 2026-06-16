// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionimagedocumentcommandruntime.h"

#include <QObject>
#include <QTest>
#include <optional>

class TestDocumentSessionImageDocumentCommandRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void forwardsSourceRoutingThroughPort();
    void forwardsPageNavigationThroughPort();
    void forwardsImageDocumentDeletionThroughPort();
};

namespace {
struct ImageCommandProbe {
    kiriview::DocumentSessionImageDocumentCommandPort port()
    {
        return kiriview::DocumentSessionImageDocumentCommandPort {
            [this](const QUrl &url) {
                sourceUrl = url;
                events.push_back(
                    url.isEmpty() ? QStringLiteral("clear-source") : QStringLiteral("set-source"));
            },
            [this]() { events.push_back(QStringLiteral("previous-page")); },
            [this]() { events.push_back(QStringLiteral("next-page")); },
            [this](int number) {
                openedPageNumber = number;
                events.push_back(QStringLiteral("open-page"));
            },
            [this](kiriview::FileDeletionMode mode) {
                deletionMode = mode;
                events.push_back(QStringLiteral("delete-displayed-file"));
            },
        };
    }

    QUrl sourceUrl;
    int openedPageNumber = 0;
    std::optional<kiriview::FileDeletionMode> deletionMode;
    QStringList events;
};
}

void TestDocumentSessionImageDocumentCommandRuntime::forwardsSourceRoutingThroughPort()
{
    ImageCommandProbe probe;
    kiriview::DocumentSessionImageDocumentCommandRuntime runtime(probe.port());
    const QUrl imageUrl(QStringLiteral("file:///tmp/image.png"));

    runtime.setSourceUrl(imageUrl);
    runtime.clearSourceUrl();

    QCOMPARE(probe.sourceUrl, QUrl());
    QCOMPARE(probe.events,
        QStringList({ QStringLiteral("set-source"), QStringLiteral("clear-source") }));
}

void TestDocumentSessionImageDocumentCommandRuntime::forwardsPageNavigationThroughPort()
{
    ImageCommandProbe probe;
    kiriview::DocumentSessionImageDocumentCommandRuntime runtime(probe.port());

    runtime.openPreviousPage();
    runtime.openNextPage();
    runtime.openImageAtPage(42);

    QCOMPARE(probe.openedPageNumber, 42);
    QCOMPARE(probe.events,
        QStringList({ QStringLiteral("previous-page"), QStringLiteral("next-page"),
            QStringLiteral("open-page") }));
}

void TestDocumentSessionImageDocumentCommandRuntime::forwardsImageDocumentDeletionThroughPort()
{
    ImageCommandProbe probe;
    kiriview::DocumentSessionImageDocumentCommandRuntime runtime(probe.port());

    runtime.deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);

    QVERIFY(probe.deletionMode.has_value());
    QCOMPARE(*probe.deletionMode, kiriview::FileDeletionMode::MoveToTrash);
    QCOMPARE(probe.events, QStringList({ QStringLiteral("delete-displayed-file") }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionImageDocumentCommandRuntime)

#include "test_documentsessionimagedocumentcommandruntime.moc"
