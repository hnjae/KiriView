// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/activenavigationthumbnailruntime.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::ActiveNavigationThumbnailRow thumbnailRow(int number, const QUrl &url,
    const QString &label, KiriView::ActiveNavigationThumbnailSourceKind sourceKind,
    bool current = false)
{
    return KiriView::ActiveNavigationThumbnailRow {
        number,
        url,
        label,
        sourceKind == KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo
                || sourceKind
                    == KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageVideo
            ? KiriView::ActiveNavigationThumbnailKind::Video
            : KiriView::ActiveNavigationThumbnailKind::Image,
        sourceKind,
        current,
    };
}

int roleForName(const QAbstractItemModel &model, const QByteArray &name)
{
    const QHash<int, QByteArray> roles = model.roleNames();
    for (auto iterator = roles.cbegin(); iterator != roles.cend(); ++iterator) {
        if (iterator.value() == name) {
            return iterator.key();
        }
    }

    return -1;
}

QVariant modelData(const QAbstractItemModel &model, int row, const QByteArray &roleName)
{
    const int role = roleForName(model, roleName);
    if (role < 0) {
        return {};
    }

    return model.data(model.index(row, 0), role);
}
}

class TestActiveNavigationThumbnailRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsSourceKeysAndBumpsGenerationOnlyForScopeChanges();
    void sameRowDemandCoalescesButBucketAndPriorityChangesAreAccepted();
    void multipleRowsKeepIndependentDemandState();
    void unsupportedRowsProjectThroughModelResultPath();
    void staleCompletionIsRejected();
    void rowResetRejectsOlderCompletion();
    void activeJobsCancelOnIdentityChangeAndSupersedingDemand();
};

void TestActiveNavigationThumbnailRuntime::buildsSourceKeysAndBumpsGenerationOnlyForScopeChanges()
{
    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.mp4"));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo),
    });

    QCOMPARE(runtime.navigationGeneration(), quint64(1));
    QCOMPARE(runtime.sourceKeyAt(0).number, 1);
    QCOMPARE(runtime.sourceKeyAt(0).url, firstUrl);
    QCOMPARE(runtime.sourceKeyAt(0).label, QStringLiteral("01.png"));
    QCOMPARE(runtime.sourceKeyAt(0).sourceKind,
        KiriView::ActiveNavigationThumbnailSourceKind::DirectImage);
    QCOMPARE(runtime.sourceKeyAt(0).navigationGeneration, quint64(1));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(1));

    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("renamed.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
        thumbnailRow(2, secondUrl, QStringLiteral("02.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(2));
    QCOMPARE(runtime.sourceKeyAt(0).label, QStringLiteral("renamed.png"));
    QCOMPARE(runtime.sourceKeyAt(0).navigationGeneration, quint64(2));
}

void TestActiveNavigationThumbnailRuntime::
    sameRowDemandCoalescesButBucketAndPriorityChangesAreAccepted()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Normal, Priority::Visible, generation));
    QCOMPARE(runtime.resultAt(0).status, KiriView::ActiveNavigationThumbnailResultStatus::Pending);

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));

    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Nearby, generation));
    QVERIFY(!runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Nearby, generation));
}

void TestActiveNavigationThumbnailRuntime::multipleRowsKeepIndependentDemandState()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
        thumbnailRow(2, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(1, firstUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(!runtime.reportDemand(2, secondUrl, Bucket::Normal, Priority::Visible, generation));
}

void TestActiveNavigationThumbnailRuntime::unsupportedRowsProjectThroughModelResultPath()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl videoUrl = localUrl(QStringLiteral("/media/clip.mp4"));
    const QUrl pageUrl = localUrl(QStringLiteral("/archive/01.png"));
    runtime.setRows({
        thumbnailRow(1, videoUrl, QStringLiteral("clip.mp4"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo, true),
        thumbnailRow(2, pageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage),
    });

    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, videoUrl, Bucket::Normal, Priority::Visible, generation));
    QVERIFY(runtime.reportDemand(2, pageUrl, Bucket::Normal, Priority::Visible, generation));

    QAbstractItemModel *model = runtime.model();
    QVERIFY(model != nullptr);
    QCOMPARE(modelData(*model, 0, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriView::ActiveNavigationThumbnailResultStatus::Unsupported));
    QCOMPARE(modelData(*model, 1, QByteArrayLiteral("thumbnailStatus")).toInt(),
        static_cast<int>(KiriView::ActiveNavigationThumbnailResultStatus::Unsupported));
    QCOMPARE(modelData(*model, 0, QByteArrayLiteral("thumbnailImageSource")).toUrl(), QUrl());
}

void TestActiveNavigationThumbnailRuntime::staleCompletionIsRejected()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    runtime.setRows({
        thumbnailRow(1, imageUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    const quint64 generation = runtime.navigationGeneration();
    QVERIFY(runtime.reportDemand(1, imageUrl, Bucket::Large, Priority::Visible, generation));

    const KiriView::ActiveNavigationThumbnailSourceKey key = runtime.sourceKeyAt(0);
    KiriView::ActiveNavigationThumbnailCompletion completion {
        key,
        Bucket::Large,
        KiriView::ActiveNavigationThumbnailResult {
            Status::Ready,
            QUrl(QStringLiteral("image://kiriview-thumbnails/current")),
        },
    };

    KiriView::ActiveNavigationThumbnailCompletion stale = completion;
    stale.sourceKey.url = localUrl(QStringLiteral("/media/other.png"));
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.number = 2;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.sourceKind = KiriView::ActiveNavigationThumbnailSourceKind::DirectVideo;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.bucket = Bucket::Normal;
    QVERIFY(!runtime.applyCompletion(stale));

    stale = completion;
    stale.sourceKey.navigationGeneration = generation + 1;
    QVERIFY(!runtime.applyCompletion(stale));

    QCOMPARE(runtime.resultAt(0).status, Status::Pending);
    QVERIFY(runtime.applyCompletion(completion));
    QCOMPARE(runtime.resultAt(0).status, Status::Ready);
    QCOMPARE(runtime.resultAt(0).imageSource, completion.result.imageSource);
}

void TestActiveNavigationThumbnailRuntime::rowResetRejectsOlderCompletion()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;
    using Status = KiriView::ActiveNavigationThumbnailResultStatus;

    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    const KiriView::ActiveNavigationThumbnailSourceKey staleKey = runtime.sourceKeyAt(0);

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QCOMPARE(runtime.navigationGeneration(), quint64(2));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);

    QVERIFY(!runtime.applyCompletion(KiriView::ActiveNavigationThumbnailCompletion {
        staleKey,
        Bucket::Normal,
        KiriView::ActiveNavigationThumbnailResult {
            Status::Ready,
            QUrl(QStringLiteral("image://kiriview-thumbnails/stale")),
        },
    }));
    QCOMPARE(runtime.resultAt(0).status, Status::NoResult);
}

void TestActiveNavigationThumbnailRuntime::activeJobsCancelOnIdentityChangeAndSupersedingDemand()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    KiriView::ActiveNavigationThumbnailRuntime runtime;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    runtime.setRows({
        thumbnailRow(1, firstUrl, QStringLiteral("01.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Normal, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(runtime.activeJobCount(), qsizetype(1));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(0));

    QVERIFY(runtime.reportDemand(
        1, firstUrl, Bucket::Large, Priority::Visible, runtime.navigationGeneration()));
    QCOMPARE(runtime.activeJobCount(), qsizetype(1));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(1));

    runtime.setRows({
        thumbnailRow(1, secondUrl, QStringLiteral("02.png"),
            KiriView::ActiveNavigationThumbnailSourceKind::DirectImage, true),
    });
    QCOMPARE(runtime.activeJobCount(), qsizetype(0));
    QCOMPARE(runtime.canceledJobCount(), qsizetype(2));
}

QTEST_GUILESS_MAIN(TestActiveNavigationThumbnailRuntime)

#include "test_activenavigationthumbnailruntime.moc"
