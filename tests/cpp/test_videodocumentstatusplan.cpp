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
    void naturalEndStopsPublicPlaybackWithoutChangingReadyStatus();
};

namespace {
KiriView::VideoDocumentStatusPlan planForMediaStatus(KiriView::VideoMediaStatus mediaStatus)
{
    return KiriView::videoDocumentStatusPlan(KiriView::VideoDocumentStatusSnapshot {
        false,
        false,
        true,
        mediaStatus,
    });
}
}

void TestVideoDocumentStatusPlan::sourceAndResolverStateTakePrecedenceOverBackendStatus()
{
    const KiriView::VideoDocumentStatusPlan noSourcePlan
        = KiriView::videoDocumentStatusPlan(KiriView::VideoDocumentStatusSnapshot {
            true,
            false,
            true,
            KiriView::VideoMediaStatus::Buffered,
        });
    QCOMPARE(noSourcePlan.status, KiriView::VideoDocumentStatus::Null);
    QVERIFY(!noSourcePlan.ended);
    QVERIFY(!noSourcePlan.stopPublicPlayback);

    const KiriView::VideoDocumentStatusPlan sourceLoadPlan
        = KiriView::videoDocumentStatusPlan(KiriView::VideoDocumentStatusSnapshot {
            false,
            true,
            true,
            KiriView::VideoMediaStatus::Invalid,
        });
    QCOMPARE(sourceLoadPlan.status, KiriView::VideoDocumentStatus::Loading);
    QVERIFY(!sourceLoadPlan.ended);
    QVERIFY(!sourceLoadPlan.stopPublicPlayback);

    const KiriView::VideoDocumentStatusPlan missingBackendPlan
        = KiriView::videoDocumentStatusPlan(KiriView::VideoDocumentStatusSnapshot {
            false,
            false,
            false,
            KiriView::VideoMediaStatus::Invalid,
        });
    QCOMPARE(missingBackendPlan.status, KiriView::VideoDocumentStatus::Loading);
    QVERIFY(!missingBackendPlan.ended);
    QVERIFY(!missingBackendPlan.stopPublicPlayback);
}

void TestVideoDocumentStatusPlan::mediaStatusMapsToPublicDocumentStatus()
{
    struct Case {
        KiriView::VideoMediaStatus mediaStatus;
        KiriView::VideoDocumentStatus documentStatus;
    };
    const std::vector<Case> cases {
        { KiriView::VideoMediaStatus::Null, KiriView::VideoDocumentStatus::Loading },
        { KiriView::VideoMediaStatus::Loading, KiriView::VideoDocumentStatus::Loading },
        { KiriView::VideoMediaStatus::Loaded, KiriView::VideoDocumentStatus::Ready },
        { KiriView::VideoMediaStatus::Stalled, KiriView::VideoDocumentStatus::Loading },
        { KiriView::VideoMediaStatus::Buffering, KiriView::VideoDocumentStatus::Ready },
        { KiriView::VideoMediaStatus::Buffered, KiriView::VideoDocumentStatus::Ready },
        { KiriView::VideoMediaStatus::Invalid, KiriView::VideoDocumentStatus::Error },
    };

    for (const Case &testCase : cases) {
        const KiriView::VideoDocumentStatusPlan plan = planForMediaStatus(testCase.mediaStatus);

        QCOMPARE(plan.status, testCase.documentStatus);
        QVERIFY(!plan.ended);
        QVERIFY(!plan.stopPublicPlayback);
    }
}

void TestVideoDocumentStatusPlan::naturalEndStopsPublicPlaybackWithoutChangingReadyStatus()
{
    const KiriView::VideoDocumentStatusPlan plan
        = planForMediaStatus(KiriView::VideoMediaStatus::EndOfMedia);

    QCOMPARE(plan.status, KiriView::VideoDocumentStatus::Ready);
    QVERIFY(plan.ended);
    QVERIFY(plan.stopPublicPlayback);
}

QTEST_GUILESS_MAIN(TestVideoDocumentStatusPlan)

#include "test_videodocumentstatusplan.moc"
