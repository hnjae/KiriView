// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentstate.h"
#include "document/imageopentransitionapplier.h"
#include "navigation/imagecontainer.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <optional>
#include <variant>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &path)
{
    QUrl url = archiveRootUrl;
    url.setPath(archiveRootUrl.path() + path);
    return url;
}

KiriView::ImageOpenStateDelta stateDelta(KiriView::ImageOpenUrlTarget sourceUrl,
    KiriView::ImageOpenDisplayedLocationTarget displayedLocation,
    KiriView::ImageOpenUrlTarget containerNavigationUrl, KiriView::ImageOpenBoolTarget loading,
    KiriView::ImageOpenStatusTarget status, KiriView::ImageOpenErrorStringTarget errorString,
    bool clearLoadingContainerNavigationUrl)
{
    return KiriView::ImageOpenStateDelta { sourceUrl, displayedLocation, containerNavigationUrl,
        loading, status, errorString, clearLoadingContainerNavigationUrl };
}

template <typename Effect> const Effect *findEffect(const KiriView::ImageDocumentEffects &effects)
{
    for (const KiriView::ImageDocumentEffect &effect : effects) {
        if (const auto *payload = std::get_if<Effect>(&effect.payload)) {
            return payload;
        }
    }
    return nullptr;
}

bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}
}

class TestImageOpenTransitionApplier : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void applicationPlanResolvesRuntimeTargetsBeforeStateMutation();
    void successfulTransitionAppliesSessionStateAndEffects();
    void errorTransitionUsesDisplayedFallbackAndProvidedError();
    void missingRuntimeContextDoesNotClearResolvedTargets();
};

void TestImageOpenTransitionApplier::applicationPlanResolvesRuntimeTargetsBeforeStateMutation()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(containerUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(imageUrl, *archiveDocument);
    const KiriView::ImageLoadSession session {
        5,
        KiriView::ImageLoadRequest::fromUrl(imageUrl, containerUrl),
        location,
    };
    KiriView::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(KiriView::ImageOpenUrlTarget::SessionImage,
        KiriView::ImageOpenDisplayedLocationTarget::Session,
        KiriView::ImageOpenUrlTarget::SessionContainerNavigation,
        KiriView::ImageOpenBoolTarget::False, KiriView::ImageOpenStatusTarget::Ready,
        KiriView::ImageOpenErrorStringTarget::Clear, true);
    transition.effects.push_back(KiriView::ImageOpenEffect::UpdatePageNavigation);

    const KiriView::ImageOpenApplicationPlan plan = KiriView::imageOpenApplicationPlan(
        transition, KiriView::ImageOpenTransitionContext::successfulImageLoad(session));

    QVERIFY(plan.stateDelta.sourceUrl.has_value());
    QCOMPARE(*plan.stateDelta.sourceUrl, imageUrl);
    QVERIFY(plan.stateDelta.displayedLocation.has_value());
    QCOMPARE(*plan.stateDelta.displayedLocation, location);
    QVERIFY(plan.stateDelta.containerNavigationUrl.has_value());
    QCOMPARE(*plan.stateDelta.containerNavigationUrl, containerUrl);
    QVERIFY(plan.stateDelta.loading.has_value());
    QCOMPARE(*plan.stateDelta.loading, false);
    QVERIFY(plan.stateDelta.status.has_value());
    QCOMPARE(*plan.stateDelta.status, KiriView::ImageDocumentStatus::Ready);
    QVERIFY(plan.stateDelta.errorString.has_value());
    QVERIFY(plan.stateDelta.errorString->isEmpty());
    QVERIFY(plan.stateDelta.clearLoadingContainerNavigationUrl);
    QVERIFY(findEffect<KiriView::UpdatePageNavigationEffect>(plan.effects) != nullptr);
}

void TestImageOpenTransitionApplier::successfulTransitionAppliesSessionStateAndEffects()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    state.setLoading(true);
    state.setErrorString(QStringLiteral("previous error"));
    changes.clear();

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(imageUrl, *archiveDocument);
    const KiriView::ImageLoadSession session {
        7,
        KiriView::ImageLoadRequest::fromUrl(archiveUrl),
        location,
    };
    KiriView::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(KiriView::ImageOpenUrlTarget::SessionImage,
        KiriView::ImageOpenDisplayedLocationTarget::Session,
        KiriView::ImageOpenUrlTarget::DerivedContainerNavigation,
        KiriView::ImageOpenBoolTarget::False, KiriView::ImageOpenStatusTarget::Ready,
        KiriView::ImageOpenErrorStringTarget::Clear, true);
    transition.effects.push_back(KiriView::ImageOpenEffect::UpdatePageNavigation);
    transition.effects.push_back(KiriView::ImageOpenEffect::ScheduleAdjacentImagePredecode);

    const KiriView::ImageDocumentEffects effects = KiriView::applyImageOpenTransition(
        state, transition, KiriView::ImageOpenTransitionContext::successfulImageLoad(session));

    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedImageLocation(), location);
    QCOMPARE(state.containerNavigationUrl(), KiriView::containerNavigationUrlForLocation(location));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(state.errorString().isEmpty());
    QVERIFY(findEffect<KiriView::UpdatePageNavigationEffect>(effects) != nullptr);
    QVERIFY(findEffect<KiriView::ScheduleAdjacentImagePredecodeEffect>(effects) != nullptr);
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::SourceUrl));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::DisplayedUrl));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::ContainerNavigation));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Loading));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Status));
}

void TestImageOpenTransitionApplier::errorTransitionUsesDisplayedFallbackAndProvidedError()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl previousImageUrl = localUrl(QStringLiteral("/images/previous.png"));
    const QUrl failedImageUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setSourceUrl(failedImageUrl);
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(previousImageUrl));
    state.setLoading(true);
    changes.clear();

    const KiriView::ImageLoadSession session {
        9,
        KiriView::ImageLoadRequest::fromUrl(failedImageUrl),
        KiriView::DisplayedImageLocation::fromUrl(failedImageUrl),
    };
    KiriView::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(KiriView::ImageOpenUrlTarget::Displayed,
        KiriView::ImageOpenDisplayedLocationTarget::Unchanged,
        KiriView::ImageOpenUrlTarget::Unchanged, KiriView::ImageOpenBoolTarget::False,
        KiriView::ImageOpenStatusTarget::Ready, KiriView::ImageOpenErrorStringTarget::Provided,
        true);
    transition.effects.push_back(KiriView::ImageOpenEffect::UpdatePageNavigation);
    transition.effects.push_back(KiriView::ImageOpenEffect::ScheduleAdjacentImagePredecode);

    const KiriView::ImageDocumentEffects effects
        = KiriView::applyImageOpenTransition(state, transition,
            KiriView::ImageOpenTransitionContext::sourceLoadError(
                session, previousImageUrl, QStringLiteral("missing")));

    QCOMPARE(state.sourceUrl(), previousImageUrl);
    QCOMPARE(state.displayedUrl(), previousImageUrl);
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(state.errorString(), QStringLiteral("missing"));
    QVERIFY(findEffect<KiriView::UpdatePageNavigationEffect>(effects) != nullptr);
    QVERIFY(findEffect<KiriView::ScheduleAdjacentImagePredecodeEffect>(effects) != nullptr);
    QVERIFY(!changes.empty());
    QCOMPARE(changes.front(), KiriView::ImageDocumentChange::Loading);
}

void TestImageOpenTransitionApplier::missingRuntimeContextDoesNotClearResolvedTargets()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl sourceUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/displayed.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/images/"));
    state.setSourceUrl(sourceUrl);
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setContainerNavigationUrl(containerUrl);
    state.setLoading(true);
    state.setErrorString(QStringLiteral("previous error"));
    changes.clear();

    KiriView::ImageOpenTransition transition;
    transition.stateDelta = stateDelta(KiriView::ImageOpenUrlTarget::SessionImage,
        KiriView::ImageOpenDisplayedLocationTarget::Session,
        KiriView::ImageOpenUrlTarget::DerivedContainerNavigation,
        KiriView::ImageOpenBoolTarget::False, KiriView::ImageOpenStatusTarget::Ready,
        KiriView::ImageOpenErrorStringTarget::Provided, true);
    transition.effects.push_back(KiriView::ImageOpenEffect::PrepareFailedContainer);

    const KiriView::ImageDocumentEffects effects
        = KiriView::applyImageOpenTransition(state, transition);

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.displayedUrl(), displayedUrl);
    QCOMPARE(state.containerNavigationUrl(), containerUrl);
    QCOMPARE(state.errorString(), QStringLiteral("previous error"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(effects.empty());
    QVERIFY(!containsChange(changes, KiriView::ImageDocumentChange::SourceUrl));
    QVERIFY(!containsChange(changes, KiriView::ImageDocumentChange::DisplayedUrl));
    QVERIFY(!containsChange(changes, KiriView::ImageDocumentChange::ContainerNavigation));
    QVERIFY(!containsChange(changes, KiriView::ImageDocumentChange::ErrorString));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Loading));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Status));
}

QTEST_GUILESS_MAIN(TestImageOpenTransitionApplier)

#include "test_imageopentransitionapplier.moc"
