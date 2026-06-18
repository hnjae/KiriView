// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmediaactivityport.h"
#include "session/documentsessiondirectmedianavigationinputport.h"
#include "session/documentsessiondirectmediascopeport.h"
#include "session/documentsessionpublicsnapshotinputport.h"
#include "session/documentsessionstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionPublicSnapshotInputPort : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void assignsRevisionsWhileBuildingFromCommittedFacts();
};

void TestDocumentSessionPublicSnapshotInputPort::assignsRevisionsWhileBuildingFromCommittedFacts()
{
    kiriview::DocumentSessionState state;
    state.setSourceIdentity(QUrl::fromLocalFile(QStringLiteral("/media/01.png")));
    state.setDocumentKind(kiriview::DocumentSessionKind::Image);
    state.setSessionErrorString(QStringLiteral("session error"));
    state.setFileDeletionInProgress(true);
    state.setActiveNavigationRevealIntent(kiriview::ActiveNavigationRevealIntent::LoadOrOpen);
    state.setActiveNavigationRevealDirection(kiriview::ActiveNavigationRevealDirection::Next);
    state.setDirectMediaNavigation(
        kiriview::DirectMediaNavigationBoundaryState { true, false, false, true, 2, 4 }, true, {});
    state.requestDirectImageCursor(QUrl::fromLocalFile(QStringLiteral("/media/02.png")));

    kiriview::DocumentSessionPublicImageLeafSnapshot image;
    image.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    image.displayedUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    image.readyForInformation = true;
    image.readyForDeletion = true;
    image.windowTitleFileName = QStringLiteral("01.png");

    kiriview::DocumentSessionPublicVideoLeafSnapshot video;
    video.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    video.ready = true;

    const kiriview::DocumentSessionDirectMediaScopePort scope(&state);
    const kiriview::DocumentSessionDirectMediaActivityPort activity(&state, &scope);
    const kiriview::DocumentSessionDirectMediaNavigationInputPort directMediaNavigation(&state);
    kiriview::DocumentSessionPublicSnapshotInputPort port(
        &state, &activity, &directMediaNavigation, &image, &video);

    const kiriview::DocumentSessionPublicSnapshotInput firstInput = port.nextInput();
    const kiriview::DocumentSessionPublicSnapshotInput secondInput = port.nextInput();

    QCOMPARE(firstInput.inputRevision, quint64(1));
    QCOMPARE(secondInput.inputRevision, quint64(2));
    QCOMPARE(firstInput.session.sourceUrl, state.sourceUrl());
    QCOMPARE(firstInput.session.documentKind, kiriview::DocumentSessionKind::Image);
    QCOMPARE(firstInput.session.sessionErrorString, QStringLiteral("session error"));
    QVERIFY(firstInput.session.fileDeletionInProgress);
    QVERIFY(firstInput.session.directImageLoadMayUseImageDocumentSourceScope);
    QCOMPARE(firstInput.session.activeNavigationRevealIntent,
        kiriview::ActiveNavigationRevealIntent::LoadOrOpen);
    QCOMPARE(firstInput.session.activeNavigationRevealDirection,
        kiriview::ActiveNavigationRevealDirection::Next);
    QVERIFY(firstInput.session.directMediaNavigation.known);
    QCOMPARE(firstInput.session.directMediaNavigation.boundaryState.currentNumber, 2);
    QCOMPARE(firstInput.session.directMediaNavigation.boundaryState.count, 4);
    QCOMPARE(firstInput.image.sourceUrl, image.sourceUrl);
    QVERIFY(firstInput.image.directImageReplacementPending);
    QVERIFY(firstInput.operations.displayedMediaOpenWithAvailable);
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicSnapshotInputPort)

#include "test_documentsessionpublicsnapshotinputport.moc"
