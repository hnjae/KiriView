// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/activenavigationprojection.h"

#include <QObject>
#include <QTest>

namespace {
void compareUnavailable(const KiriView::ActiveNavigationSnapshot &snapshot)
{
    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.known);
    QVERIFY(!snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 0);
    QCOMPARE(snapshot.count, 0);
}

void compareUnknown(const KiriView::ActiveNavigationSnapshot &snapshot)
{
    QVERIFY(snapshot.available);
    QVERIFY(!snapshot.known);
    QVERIFY(!snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 0);
    QCOMPARE(snapshot.count, 0);
}
}

class TestActiveNavigationProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unavailableSourceProjectsUnavailable();
    void unknownMediaStateProjectsAvailableButUnknown();
    void validMediaBoundaryStateProjectsReadoutAndActions();
    void invalidMediaNumbersNormalizeToUnknown();
    void validImageDocumentSnapshotProjectsSpreadAwareBoundaries();
    void invalidImagePageValuesNormalizeToUnknown();
    void deletionMaskingDisablesDispatchButKeepsKnownReadout();
    void boundaryScopeMapsSourceKind();
};

void TestActiveNavigationProjection::unavailableSourceProjectsUnavailable()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::None, {}, {}, false);

    compareUnavailable(snapshot);
}

void TestActiveNavigationProjection::unknownMediaStateProjectsAvailableButUnknown()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, {}, {}, false);

    compareUnknown(snapshot);
}

void TestActiveNavigationProjection::validMediaBoundaryStateProjectsReadoutAndActions()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::MediaActiveNavigationInput {
            KiriView::MediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, false);

    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QVERIFY(snapshot.editable);
    QVERIFY(snapshot.canOpenPrevious);
    QVERIFY(snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 2);
    QCOMPARE(snapshot.count, 4);
}

void TestActiveNavigationProjection::invalidMediaNumbersNormalizeToUnknown()
{
    const KiriView::ActiveNavigationSnapshot zeroCurrent = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::MediaActiveNavigationInput {
            KiriView::MediaNavigationBoundaryState { true, true, false, false, 0, 4 }, true },
        {}, false);
    compareUnknown(zeroCurrent);

    const KiriView::ActiveNavigationSnapshot outOfRange = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::MediaActiveNavigationInput {
            KiriView::MediaNavigationBoundaryState { true, false, false, true, 5, 4 }, true },
        {}, false);
    compareUnknown(outOfRange);
}

void TestActiveNavigationProjection::validImageDocumentSnapshotProjectsSpreadAwareBoundaries()
{
    const KiriView::ActiveNavigationSnapshot firstSpread = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        KiriView::ImageDocumentActiveNavigationInput { 1, 2, 5 }, false);
    QVERIFY(firstSpread.available);
    QVERIFY(firstSpread.known);
    QVERIFY(firstSpread.editable);
    QVERIFY(!firstSpread.canOpenPrevious);
    QVERIFY(firstSpread.canOpenNext);
    QVERIFY(firstSpread.atKnownFirst);
    QVERIFY(!firstSpread.atKnownLast);
    QCOMPARE(firstSpread.currentNumber, 1);
    QCOMPARE(firstSpread.count, 5);

    const KiriView::ActiveNavigationSnapshot lastSpread = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        KiriView::ImageDocumentActiveNavigationInput { 4, 5, 5 }, false);
    QVERIFY(lastSpread.canOpenPrevious);
    QVERIFY(!lastSpread.canOpenNext);
    QVERIFY(!lastSpread.atKnownFirst);
    QVERIFY(lastSpread.atKnownLast);
    QCOMPARE(lastSpread.currentNumber, 4);
    QCOMPARE(lastSpread.count, 5);
}

void TestActiveNavigationProjection::invalidImagePageValuesNormalizeToUnknown()
{
    const KiriView::ActiveNavigationSnapshot reversedSpread = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        KiriView::ImageDocumentActiveNavigationInput { 3, 2, 5 }, false);
    compareUnknown(reversedSpread);

    const KiriView::ActiveNavigationSnapshot lastPastCount = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        KiriView::ImageDocumentActiveNavigationInput { 4, 6, 5 }, false);
    compareUnknown(lastPastCount);
}

void TestActiveNavigationProjection::deletionMaskingDisablesDispatchButKeepsKnownReadout()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::MediaActiveNavigationInput {
            KiriView::MediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, true);

    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QVERIFY(!snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 2);
    QCOMPARE(snapshot.count, 4);
}

void TestActiveNavigationProjection::boundaryScopeMapsSourceKind()
{
    QVERIFY(
        KiriView::activeNavigationBoundaryScopeForSource(KiriView::ActiveNavigationSourceKind::None)
        == KiriView::ActiveNavigationBoundaryScope::None);
    QVERIFY(KiriView::activeNavigationBoundaryScopeForSource(
                KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia)
        == KiriView::ActiveNavigationBoundaryScope::Media);
    QVERIFY(KiriView::activeNavigationBoundaryScopeForSource(
                KiriView::ActiveNavigationSourceKind::ImageDocumentPages)
        == KiriView::ActiveNavigationBoundaryScope::ImageDocument);
}

QTEST_GUILESS_MAIN(TestActiveNavigationProjection)

#include "test_activenavigationprojection.moc"
