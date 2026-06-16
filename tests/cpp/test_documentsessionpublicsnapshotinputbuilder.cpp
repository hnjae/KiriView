// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicsnapshotinputbuilder.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionPublicSnapshotInputBuilder : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsInputFromCommittedSessionAndLeafFacts();
    void derivesVideoOpenWithAvailabilityFromVideoFacts();
};

void TestDocumentSessionPublicSnapshotInputBuilder::buildsInputFromCommittedSessionAndLeafFacts()
{
    kiriview::DocumentSessionPublicSnapshotInputBuilderInput builderInput;
    builderInput.inputRevision = 42;
    builderInput.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    builderInput.session.documentKind = kiriview::DocumentSessionKind::Image;
    builderInput.session.sessionErrorString = QStringLiteral("session error");
    builderInput.session.fileDeletionInProgress = true;
    builderInput.session.directImageLoadMayUseImageDocumentSourceScope = true;
    builderInput.session.activeNavigationRevealIntent
        = kiriview::ActiveNavigationRevealIntent::AdjacentNavigation;
    builderInput.session.activeNavigationRevealDirection
        = kiriview::ActiveNavigationRevealDirection::Next;
    builderInput.session.directMediaNavigation = kiriview::DirectMediaActiveNavigationInput {
        kiriview::DirectMediaNavigationBoundaryState { true, false, false, true, 2, 2 },
        true,
    };
    builderInput.image.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    builderInput.image.displayedUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    builderInput.image.readyForInformation = true;
    builderInput.directMediaCursor.pendingUrl
        = QUrl::fromLocalFile(QStringLiteral("/media/02.png"));

    const kiriview::DocumentSessionPublicSnapshotInput input
        = kiriview::buildDocumentSessionPublicSnapshotInput(builderInput);

    QCOMPARE(input.inputRevision, quint64(42));
    QCOMPARE(input.session.sourceUrl, builderInput.session.sourceUrl);
    QCOMPARE(input.session.documentKind, kiriview::DocumentSessionKind::Image);
    QCOMPARE(input.session.sessionErrorString, QStringLiteral("session error"));
    QVERIFY(input.session.fileDeletionInProgress);
    QVERIFY(input.session.directImageLoadMayUseImageDocumentSourceScope);
    QCOMPARE(input.session.activeNavigationRevealIntent,
        kiriview::ActiveNavigationRevealIntent::AdjacentNavigation);
    QCOMPARE(input.session.activeNavigationRevealDirection,
        kiriview::ActiveNavigationRevealDirection::Next);
    QVERIFY(input.session.directMediaNavigation.known);
    QCOMPARE(input.image.sourceUrl, builderInput.image.sourceUrl);
    QVERIFY(input.image.directImageReplacementPending);
    QVERIFY(input.operations.displayedMediaOpenWithAvailable);
}

void TestDocumentSessionPublicSnapshotInputBuilder::derivesVideoOpenWithAvailabilityFromVideoFacts()
{
    kiriview::DocumentSessionPublicSnapshotInputBuilderInput builderInput;
    builderInput.session.documentKind = kiriview::DocumentSessionKind::Video;
    builderInput.video.ready = true;
    builderInput.video.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));

    const kiriview::DocumentSessionPublicSnapshotInput input
        = kiriview::buildDocumentSessionPublicSnapshotInput(builderInput);

    QVERIFY(input.operations.displayedMediaOpenWithAvailable);
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicSnapshotInputBuilder)

#include "test_documentsessionpublicsnapshotinputbuilder.moc"
