// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentstatusplan.h"

#include <QObject>
#include <QTest>
#include <vector>

class TestVideoDocumentStatusPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceAndResolverStateTakePrecedenceOverBackendStatus();
    void mediaStatusMapsToPublicDocumentStatus();
    void naturalEndClearsPlayingWithoutChangingReadyStatus();
};

namespace {
kiriview::VideoDocumentStatusPlan planForMediaStatus(kiriview::VideoMediaStatus mediaStatus)
{
    return kiriview::videoDocumentStatusPlan(kiriview::VideoDocumentStatusSnapshot {
        false,
        false,
        true,
        mediaStatus,
    });
}
}

void TestVideoDocumentStatusPlan::sourceAndResolverStateTakePrecedenceOverBackendStatus()
{
    const kiriview::VideoDocumentStatusPlan noSourcePlan
        = kiriview::videoDocumentStatusPlan(kiriview::VideoDocumentStatusSnapshot {
            true,
            false,
            true,
            kiriview::VideoMediaStatus::Buffered,
        });
    QCOMPARE(noSourcePlan.status, kiriview::VideoDocumentStatus::Null);
    QVERIFY(!noSourcePlan.mediaEnded);
    QVERIFY(!noSourcePlan.clearPlaying);

    const kiriview::VideoDocumentStatusPlan sourceLoadPlan
        = kiriview::videoDocumentStatusPlan(kiriview::VideoDocumentStatusSnapshot {
            false,
            true,
            true,
            kiriview::VideoMediaStatus::Invalid,
        });
    QCOMPARE(sourceLoadPlan.status, kiriview::VideoDocumentStatus::Loading);
    QVERIFY(!sourceLoadPlan.mediaEnded);
    QVERIFY(!sourceLoadPlan.clearPlaying);

    const kiriview::VideoDocumentStatusPlan missingBackendPlan
        = kiriview::videoDocumentStatusPlan(kiriview::VideoDocumentStatusSnapshot {
            false,
            false,
            false,
            kiriview::VideoMediaStatus::Invalid,
        });
    QCOMPARE(missingBackendPlan.status, kiriview::VideoDocumentStatus::Loading);
    QVERIFY(!missingBackendPlan.mediaEnded);
    QVERIFY(!missingBackendPlan.clearPlaying);
}

void TestVideoDocumentStatusPlan::mediaStatusMapsToPublicDocumentStatus()
{
    struct Case
    {
        kiriview::VideoMediaStatus mediaStatus;
        kiriview::VideoDocumentStatus documentStatus;
    };
    const std::vector<Case> cases {
        { kiriview::VideoMediaStatus::Null, kiriview::VideoDocumentStatus::Loading },
        { kiriview::VideoMediaStatus::Loading, kiriview::VideoDocumentStatus::Loading },
        { kiriview::VideoMediaStatus::Loaded, kiriview::VideoDocumentStatus::Ready },
        { kiriview::VideoMediaStatus::Stalled, kiriview::VideoDocumentStatus::Loading },
        { kiriview::VideoMediaStatus::Buffering, kiriview::VideoDocumentStatus::Ready },
        { kiriview::VideoMediaStatus::Buffered, kiriview::VideoDocumentStatus::Ready },
        { kiriview::VideoMediaStatus::Invalid, kiriview::VideoDocumentStatus::Error },
    };

    for (const Case& testCase : cases) {
        const kiriview::VideoDocumentStatusPlan plan = planForMediaStatus(testCase.mediaStatus);

        QCOMPARE(plan.status, testCase.documentStatus);
        QVERIFY(!plan.mediaEnded);
        QVERIFY(!plan.clearPlaying);
    }
}

void TestVideoDocumentStatusPlan::naturalEndClearsPlayingWithoutChangingReadyStatus()
{
    const kiriview::VideoDocumentStatusPlan plan
        = planForMediaStatus(kiriview::VideoMediaStatus::EndOfMedia);

    QCOMPARE(plan.status, kiriview::VideoDocumentStatus::Ready);
    QVERIFY(plan.mediaEnded);
    QVERIFY(plan.clearPlaying);
}

QTEST_GUILESS_MAIN(TestVideoDocumentStatusPlan)

#include "test_videodocumentstatusplan.moc"
